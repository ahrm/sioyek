#include "djvu_document.h"

#include <libdjvu/ddjvuapi.h>
#include <libdjvu/miniexp.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <new>

namespace {

constexpr float POINTS_PER_INCH = 72.0f;
constexpr float COURIER_GLYPH_ADVANCE_EM = 0.6f;

struct DjvuSynchronization {
    std::mutex queue_mutex;
    std::mutex wait_mutex;
    std::condition_variable message_available;
    std::atomic<std::uint64_t> message_generation = 0;
    std::atomic<std::uint64_t> processed_generation = 0;
};

struct DjvuDocument {
    fz_document super;
    ddjvu_context_t* context = nullptr;
    ddjvu_document_t* document = nullptr;
    ddjvu_format_t* pixel_format = nullptr;
    fz_font* text_font = nullptr;
    fz_buffer* source = nullptr;
    fz_archive* directory = nullptr;
    DjvuSynchronization* synchronization = nullptr;
    int page_count = 0;
    char** page_ids = nullptr;
    char** page_labels = nullptr;
    char* title = nullptr;
    char* author = nullptr;
    char* subject = nullptr;
    char* creator = nullptr;
    char* producer = nullptr;
    char* creation_date = nullptr;
    char* modification_date = nullptr;
};

struct DjvuPage {
    fz_page super;
    DjvuDocument* document = nullptr;
    ddjvu_page_t* page = nullptr;
    ddjvu_pageinfo_t info = {};
    int pixel_width = 0;
    int pixel_height = 0;
    int dpi = 300;
};

struct DjvuImage {
    fz_image super;
    fz_page* page = nullptr;
};

bool is_symbol(miniexp_t expression, const char* name) {
    return miniexp_symbolp(expression)
        && std::strcmp(miniexp_to_name(expression), name) == 0;
}

bool is_failed_expression(miniexp_t expression) {
    return is_symbol(expression, "failed") || is_symbol(expression, "stopped");
}

void publish_generation(DjvuSynchronization* synchronization,
    std::atomic<std::uint64_t>* generation) {
    {
        std::lock_guard<std::mutex> lock(synchronization->wait_mutex);
        generation->fetch_add(1, std::memory_order_release);
    }
    synchronization->message_available.notify_all();
}

void notify_djvu_message(ddjvu_context_t*, void* closure) {
    auto* synchronization = static_cast<DjvuSynchronization*>(closure);
    publish_generation(synchronization,
        &synchronization->message_generation);
}

void send_indirect_stream(fz_context* context, DjvuDocument* document,
    int stream_id, const char* name) {
    if (!document->directory || !name || !*name) {
        ddjvu_stream_close(document->document, stream_id, TRUE);
        fz_throw(context, FZ_ERROR_FORMAT,
            "DjVu document references unavailable component '%s'",
            name ? name : "");
    }

    fz_buffer* component = nullptr;
    int failed = 0;
    fz_try(context) {
        component = fz_read_archive_entry(context, document->directory, name);
        size_t size = 0;
        unsigned char* data = nullptr;
        size = fz_buffer_storage(context, component, &data);
        ddjvu_stream_write(document->document, stream_id,
            reinterpret_cast<const char*>(data), size);
        ddjvu_stream_close(document->document, stream_id, FALSE);
    }
    fz_catch(context) {
        failed = 1;
        ddjvu_stream_close(document->document, stream_id, TRUE);
    }
    fz_drop_buffer(context, component);

    if (failed) {
        fz_throw(context, FZ_ERROR_FORMAT,
            "Could not read DjVu component '%s'", name);
    }
}

void process_messages(fz_context* context, DjvuDocument* document, bool wait) {
    auto* synchronization = document->synchronization;
    const std::uint64_t message_generation =
        synchronization->message_generation.load(std::memory_order_acquire);
    const std::uint64_t processed_generation =
        synchronization->processed_generation.load(std::memory_order_acquire);
    bool processed_message = false;
    bool has_error = false;
    char error_message[512] = {};
    fz_var(processed_message);
    fz_var(has_error);

    synchronization->queue_mutex.lock();
    fz_try(context) {
        while (const ddjvu_message_t* message =
                ddjvu_message_peek(document->context)) {
            processed_message = true;
            if (message->m_any.tag == DDJVU_NEWSTREAM
                && message->m_any.document == document->document) {
                const auto& stream = message->m_newstream;
                if (stream.streamid != 0) {
                    send_indirect_stream(context, document,
                        stream.streamid, stream.name);
                }
            }
            else if (message->m_any.tag == DDJVU_ERROR
                && (!message->m_any.document
                    || message->m_any.document == document->document)) {
                fz_strlcpy(error_message,
                    message->m_error.message
                        ? message->m_error.message
                        : "DjVu decoding failed",
                    sizeof(error_message));
                has_error = true;
            }

            ddjvu_message_pop(document->context);
        }
        if (processed_message) {
            publish_generation(synchronization,
                &synchronization->processed_generation);
        }
    }
    fz_always(context) {
        synchronization->queue_mutex.unlock();
    }
    fz_catch(context) {
        fz_rethrow(context);
    }

    if (has_error) {
        fz_throw(context, FZ_ERROR_FORMAT, "%s", error_message);
    }
    if (!wait || processed_message
        || synchronization->processed_generation.load(
               std::memory_order_acquire)
            != processed_generation) {
        return;
    }

    std::unique_lock<std::mutex> lock(synchronization->wait_mutex);
    synchronization->message_available.wait(lock, [&] {
        return synchronization->message_generation.load(
                   std::memory_order_acquire)
                != message_generation
            || synchronization->processed_generation.load(
                   std::memory_order_acquire)
                != processed_generation;
    });
}

void wait_for_document(fz_context* context, DjvuDocument* document) {
    while (!ddjvu_document_decoding_done(document->document)) {
        process_messages(context, document, true);
    }
    process_messages(context, document, false);
    if (ddjvu_document_decoding_error(document->document)) {
        fz_throw(context, FZ_ERROR_FORMAT, "Could not decode DjVu document");
    }
}

void wait_for_page(fz_context* context, DjvuDocument* document,
    ddjvu_page_t* page) {
    while (!ddjvu_page_decoding_done(page)) {
        process_messages(context, document, true);
    }
    process_messages(context, document, false);
    if (ddjvu_page_decoding_error(page)) {
        fz_throw(context, FZ_ERROR_FORMAT, "Could not decode DjVu page");
    }
}

ddjvu_pageinfo_t get_page_info(fz_context* context, DjvuDocument* document,
    int page_number) {
    ddjvu_pageinfo_t info = {};
    ddjvu_status_t status;
    while ((status = ddjvu_document_get_pageinfo(
                document->document, page_number, &info)) < DDJVU_JOB_OK) {
        process_messages(context, document, true);
    }
    if (status >= DDJVU_JOB_FAILED) {
        fz_throw(context, FZ_ERROR_FORMAT,
            "Could not read information for DjVu page %d", page_number + 1);
    }
    return info;
}

miniexp_t get_page_text(fz_context* context, DjvuDocument* document,
    int page_number) {
    miniexp_t expression;
    while ((expression = ddjvu_document_get_pagetext(
                document->document, page_number, nullptr)) == miniexp_dummy) {
        process_messages(context, document, true);
    }
    process_messages(context, document, false);
    return expression;
}

miniexp_t get_page_annotations(fz_context* context, DjvuDocument* document,
    int page_number) {
    miniexp_t expression;
    while ((expression = ddjvu_document_get_pageanno(
                document->document, page_number)) == miniexp_dummy) {
        process_messages(context, document, true);
    }
    process_messages(context, document, false);
    return expression;
}

miniexp_t get_document_annotations(fz_context* context,
    DjvuDocument* document) {
    miniexp_t expression;
    while ((expression = ddjvu_document_get_anno(
                document->document, TRUE)) == miniexp_dummy) {
        process_messages(context, document, true);
    }
    process_messages(context, document, false);
    return expression;
}

miniexp_t get_outline(fz_context* context, DjvuDocument* document) {
    miniexp_t expression;
    while ((expression = ddjvu_document_get_outline(document->document))
        == miniexp_dummy) {
        process_messages(context, document, true);
    }
    process_messages(context, document, false);
    return expression;
}

void free_page_names(fz_context* context, DjvuDocument* document) {
    if (document->page_ids) {
        for (int i = 0; i < document->page_count; ++i) {
            fz_free(context, document->page_ids[i]);
        }
        fz_free(context, document->page_ids);
    }
    if (document->page_labels) {
        for (int i = 0; i < document->page_count; ++i) {
            fz_free(context, document->page_labels[i]);
        }
        fz_free(context, document->page_labels);
    }
}

void drop_document(fz_context* context, fz_document* document_) {
    auto* document = reinterpret_cast<DjvuDocument*>(document_);
    free_page_names(context, document);
    fz_free(context, document->title);
    fz_free(context, document->author);
    fz_free(context, document->subject);
    fz_free(context, document->creator);
    fz_free(context, document->producer);
    fz_free(context, document->creation_date);
    fz_free(context, document->modification_date);
    fz_drop_font(context, document->text_font);
    if (document->pixel_format) {
        ddjvu_format_release(document->pixel_format);
    }
    if (document->context) {
        ddjvu_message_set_callback(document->context, nullptr, nullptr);
    }
    if (document->document) {
        ddjvu_document_release(document->document);
    }
    if (document->context) {
        ddjvu_context_release(document->context);
    }
    delete document->synchronization;
    fz_drop_archive(context, document->directory);
    fz_drop_buffer(context, document->source);
}

int count_pages(fz_context*, fz_document* document_, int chapter) {
    if (chapter != 0) {
        return 0;
    }
    return reinterpret_cast<DjvuDocument*>(document_)->page_count;
}

void get_unrotated_page_size(DjvuPage* page, int* width, int* height) {
    *width = page->pixel_width;
    *height = page->pixel_height;
    if (page->info.rotation & 1) {
        std::swap(*width, *height);
    }
}

bool get_unrotated_djvu_rect(DjvuPage* page, int x_min, int y_min,
    int x_max, int y_max, fz_rect* output) {
    int unrotated_width = 0;
    int unrotated_height = 0;
    get_unrotated_page_size(page, &unrotated_width, &unrotated_height);

    // Hidden-text zones come from OCR and are not always well formed. Keep
    // corrupt zones from producing enormous or off-page MuPDF text quads.
    x_min = std::clamp(x_min, 0, unrotated_width);
    x_max = std::clamp(x_max, 0, unrotated_width);
    y_min = std::clamp(y_min, 0, unrotated_height);
    y_max = std::clamp(y_max, 0, unrotated_height);
    if (x_max <= x_min || y_max <= y_min) {
        return false;
    }

    const float scale = POINTS_PER_INCH / std::max(1, page->dpi);
    output->x0 = x_min * scale;
    output->x1 = x_max * scale;
    output->y0 = (unrotated_height - y_max) * scale;
    output->y1 = (unrotated_height - y_min) * scale;
    return true;
}

fz_matrix get_hidden_text_rotation(DjvuPage* page) {
    int unrotated_width = 0;
    int unrotated_height = 0;
    get_unrotated_page_size(page, &unrotated_width, &unrotated_height);
    const float scale = POINTS_PER_INCH / std::max(1, page->dpi);
    const float width = unrotated_width * scale;
    const float height = unrotated_height * scale;
    switch (page->info.rotation & 3) {
    case DDJVU_ROTATE_90:
        return { 0, -1, 1, 0, 0, width };
    case DDJVU_ROTATE_180:
        return { -1, 0, 0, -1, width, height };
    case DDJVU_ROTATE_270:
        return { 0, 1, -1, 0, height, 0 };
    default:
        return fz_identity;
    }
}

bool map_annotation_rect(DjvuPage* page, int x_min, int y_min, int x_max,
    int y_max, fz_rect* output) {
    fz_rect unrotated;
    if (!get_unrotated_djvu_rect(page, x_min, y_min, x_max, y_max,
            &unrotated)) {
        return false;
    }
    *output = fz_transform_rect(unrotated, get_hidden_text_rotation(page));
    return true;
}

fz_rect bound_page(fz_context*, fz_page* page_, fz_box_type) {
    auto* page = reinterpret_cast<DjvuPage*>(page_);
    const float scale = POINTS_PER_INCH / std::max(1, page->dpi);
    return { 0, 0, page->pixel_width * scale, page->pixel_height * scale };
}

void emit_text_leaf(fz_context* context, DjvuPage* page,
    miniexp_t expression, fz_text* text, fz_font* font) {
    if (miniexp_length(expression) < 6) {
        return;
    }
    for (int i = 1; i <= 4; ++i) {
        if (!miniexp_numberp(miniexp_nth(i, expression))) {
            return;
        }
    }

    miniexp_t content = miniexp_nth(5, expression);
    if (!miniexp_stringp(content)) {
        const int length = miniexp_length(expression);
        for (int i = 5; i < length; ++i) {
            miniexp_t child = miniexp_nth(i, expression);
            if (miniexp_consp(child)) {
                emit_text_leaf(context, page, child, text, font);
            }
        }
        return;
    }

    const char* string = nullptr;
    const size_t byte_length = miniexp_to_lstr(content, &string);
    if (!string || byte_length == 0) {
        return;
    }

    fz_rect rectangle;
    if (!get_unrotated_djvu_rect(page,
        miniexp_to_int(miniexp_nth(1, expression)),
        miniexp_to_int(miniexp_nth(2, expression)),
        miniexp_to_int(miniexp_nth(3, expression)),
        miniexp_to_int(miniexp_nth(4, expression)), &rectangle)) {
        return;
    }

    int character_count = 0;
    const char* cursor = string;
    const char* end = string + byte_length;
    while (cursor < end) {
        int rune = 0;
        const int consumed = fz_chartorune(&rune, cursor);
        if (consumed <= 0 || cursor + consumed > end) {
            break;
        }
        cursor += consumed;
        ++character_count;
    }
    if (character_count == 0) {
        return;
    }

    const float character_width = (rectangle.x1 - rectangle.x0)
        / character_count;
    const float rectangle_height = std::max(1.0f,
        rectangle.y1 - rectangle.y0);
    const float ascender = fz_font_ascender(context, font);
    const float descender = fz_font_descender(context, font);
    const float vertical_scale = rectangle_height
        / std::max(0.1f, ascender - descender);
    const float baseline = rectangle.y0 + ascender * vertical_scale;
    const fz_matrix page_rotation = get_hidden_text_rotation(page);
    cursor = string;
    int character_index = 0;
    while (cursor < end) {
        int rune = 0;
        const int consumed = fz_chartorune(&rune, cursor);
        if (consumed <= 0 || cursor + consumed > end) {
            break;
        }
        cursor += consumed;

        const fz_matrix unrotated_transform = {
            character_width / COURIER_GLYPH_ADVANCE_EM, 0,
            0, -vertical_scale,
            rectangle.x0 + character_index * character_width, baseline
        };
        const fz_matrix transform = fz_concat(unrotated_transform,
            page_rotation);
        fz_show_glyph(context, text, font, transform,
            fz_encode_character(context, font, rune), rune,
            0, 0, FZ_BIDI_LTR, FZ_LANG_UNSET);
        ++character_index;
    }
}

void emit_hidden_text(fz_context* context, DjvuPage* page, fz_device* device,
    fz_matrix transform) {
    miniexp_t expression = get_page_text(context, page->document,
        page->super.number);
    if (expression == miniexp_nil || is_failed_expression(expression)) {
        if (expression != miniexp_nil) {
            ddjvu_miniexp_release(page->document->document, expression);
        }
        return;
    }

    fz_text* text = nullptr;
    fz_var(text);
    fz_try(context) {
        text = fz_new_text(context);
        emit_text_leaf(context, page, expression, text,
            page->document->text_font);
        if (text->head) {
            fz_ignore_text(context, device, text, transform);
        }
    }
    fz_always(context) {
        fz_drop_text(context, text);
        ddjvu_miniexp_release(page->document->document, expression);
    }
    fz_catch(context) {
        fz_rethrow(context);
    }
}

fz_pixmap* get_djvu_pixmap(fz_context* context, fz_image* image_,
    fz_irect* subarea, int, int, int* l2factor) {
    auto* image = reinterpret_cast<DjvuImage*>(image_);
    auto* page = reinterpret_cast<DjvuPage*>(image->page);
    const int factor = l2factor ? std::clamp(*l2factor, 0, 6) : 0;
    int render_width = std::max(1,
        (page->pixel_width + (1 << factor) - 1) >> factor);
    int render_height = std::max(1,
        (page->pixel_height + (1 << factor) - 1) >> factor);
    fz_var(render_width);
    fz_var(render_height);
    fz_pixmap* pixmap = fz_new_pixmap(context, fz_device_rgb(context),
        render_width, render_height, nullptr, 0);

    fz_try(context) {
        fz_clear_pixmap_with_value(context, pixmap, 255);
        // pagerect describes the scaled full page in output coordinates;
        // renderrect selects the part of that scaled page to copy. Using the
        // source dimensions here would decode only the top-left corner when
        // MuPDF asks for a downsampled image.
        ddjvu_rect_t page_rectangle = { 0, 0,
            static_cast<unsigned int>(render_width),
            static_cast<unsigned int>(render_height) };
        ddjvu_rect_t render_rectangle = { 0, 0,
            static_cast<unsigned int>(render_width),
            static_cast<unsigned int>(render_height) };
        const int rendered = ddjvu_page_render(page->page, DDJVU_RENDER_COLOR,
                &page_rectangle, &render_rectangle,
                page->document->pixel_format, pixmap->stride,
                reinterpret_cast<char*>(pixmap->samples));
        if (!rendered
            && ddjvu_page_get_type(page->page) != DDJVU_PAGETYPE_UNKNOWN) {
            fz_throw(context, FZ_ERROR_FORMAT, "Could not render DjVu page");
        }
    }
    fz_catch(context) {
        fz_drop_pixmap(context, pixmap);
        fz_rethrow(context);
    }

    if (subarea) {
        *subarea = { 0, 0, page->pixel_width, page->pixel_height };
    }
    if (l2factor) {
        *l2factor = 0;
    }
    return pixmap;
}

size_t get_djvu_image_size(fz_context*, fz_image*) {
    return sizeof(DjvuImage);
}

void drop_djvu_image(fz_context* context, fz_image* image_) {
    auto* image = reinterpret_cast<DjvuImage*>(image_);
    fz_drop_page(context, image->page);
}

fz_image* new_djvu_image(fz_context* context, fz_page* page_) {
    auto* page = reinterpret_cast<DjvuPage*>(page_);
    auto* image = fz_new_derived_image(context,
        page->pixel_width, page->pixel_height, 8,
        fz_device_rgb(context), page->dpi, page->dpi, 1, 0,
        nullptr, nullptr, nullptr, DjvuImage,
        get_djvu_pixmap, get_djvu_image_size, drop_djvu_image);
    image->page = fz_keep_page(context, page_);
    return reinterpret_cast<fz_image*>(image);
}

void run_page(fz_context* context, fz_page* page_, fz_device* device,
    fz_matrix transform, fz_cookie*) {
    auto* page = reinterpret_cast<DjvuPage*>(page_);
    fz_image* image = nullptr;
    fz_var(image);

    fz_try(context) {
        image = new_djvu_image(context, page_);
        const fz_rect bounds = bound_page(context, page_, FZ_MEDIA_BOX);
        const fz_matrix image_matrix = fz_concat(
            fz_scale(bounds.x1 - bounds.x0, bounds.y1 - bounds.y0),
            transform);
        fz_fill_image(context, device, image, image_matrix, 1,
            fz_default_color_params);
        emit_hidden_text(context, page, device, transform);
    }
    fz_always(context) {
        fz_drop_image(context, image);
    }
    fz_catch(context) {
        fz_rethrow(context);
    }
}

void drop_page(fz_context*, fz_page* page_) {
    auto* page = reinterpret_cast<DjvuPage*>(page_);
    if (page->page) {
        ddjvu_page_release(page->page);
    }
}

int resolve_page_reference(DjvuDocument* document, const char* reference,
    int current_page) {
    if (!reference || !*reference) {
        return -1;
    }

    const char* begin = reference;
    while (*begin && std::isspace(static_cast<unsigned char>(*begin))) {
        ++begin;
    }
    const char* finish = reference + std::strlen(reference);
    while (finish > begin
        && std::isspace(static_cast<unsigned char>(finish[-1]))) {
        --finish;
    }
    if (finish == begin) {
        return -1;
    }

    char* end = nullptr;
    const long number = std::strtol(begin, &end, 10);
    if (end == finish) {
        long page = number - 1;
        if (*begin == '+' || *begin == '-') {
            page = current_page + number;
        }
        if (page >= 0 && page < document->page_count) {
            return static_cast<int>(page);
        }
    }

    for (int i = 0; i < document->page_count; ++i) {
        const char* id = document->page_ids ? document->page_ids[i] : nullptr;
        const char* label = document->page_labels
            ? document->page_labels[i]
            : nullptr;
        const size_t length = static_cast<size_t>(finish - begin);
        if ((id && std::strlen(id) == length
                && std::memcmp(id, begin, length) == 0)
            || (label && std::strlen(label) == length
                && std::memcmp(label, begin, length) == 0)) {
            return i;
        }
    }
    return -1;
}

void normalize_uri(DjvuDocument* document, const char* source,
    int current_page, char* output, size_t output_size) {
    if (!source) {
        output[0] = '\0';
        return;
    }
    if (source[0] == '#') {
        const int page = resolve_page_reference(document, source + 1,
            current_page);
        if (page >= 0) {
            std::snprintf(output, output_size, "#%d", page + 1);
            return;
        }
    }
    fz_strlcpy(output, source, output_size);
}

const char* expression_string(miniexp_t expression) {
    if (miniexp_stringp(expression)) {
        return miniexp_to_str(expression);
    }
    if (miniexp_consp(expression) && is_symbol(miniexp_car(expression), "url")
        && miniexp_stringp(miniexp_nth(1, expression))) {
        return miniexp_to_str(miniexp_nth(1, expression));
    }
    return nullptr;
}

bool hyperlink_rectangle(DjvuPage* page, miniexp_t area, fz_rect* rectangle) {
    if (!miniexp_consp(area) || !miniexp_symbolp(miniexp_car(area))) {
        return false;
    }
    const char* shape = miniexp_to_name(miniexp_car(area));
    const int length = miniexp_length(area);

    int x_min = 0;
    int y_min = 0;
    int x_max = 0;
    int y_max = 0;
    if ((!std::strcmp(shape, "rect") || !std::strcmp(shape, "oval")
            || !std::strcmp(shape, "text"))
        && length >= 5) {
        x_min = miniexp_to_int(miniexp_nth(1, area));
        y_min = miniexp_to_int(miniexp_nth(2, area));
        x_max = x_min + miniexp_to_int(miniexp_nth(3, area));
        y_max = y_min + miniexp_to_int(miniexp_nth(4, area));
    }
    else if ((!std::strcmp(shape, "poly") || !std::strcmp(shape, "line"))
        && length >= 5) {
        x_min = x_max = miniexp_to_int(miniexp_nth(1, area));
        y_min = y_max = miniexp_to_int(miniexp_nth(2, area));
        for (int i = 3; i + 1 < length; i += 2) {
            const int x = miniexp_to_int(miniexp_nth(i, area));
            const int y = miniexp_to_int(miniexp_nth(i + 1, area));
            x_min = std::min(x_min, x);
            x_max = std::max(x_max, x);
            y_min = std::min(y_min, y);
            y_max = std::max(y_max, y);
        }
    }
    else {
        return false;
    }

    return map_annotation_rect(page, x_min, y_min, x_max, y_max, rectangle);
}

fz_link* load_links(fz_context* context, fz_page* page_) {
    auto* page = reinterpret_cast<DjvuPage*>(page_);
    miniexp_t annotations = miniexp_nil;
    miniexp_t* hyperlinks = nullptr;
    fz_link* head = nullptr;
    fz_link* tail = nullptr;
    fz_var(annotations);
    fz_var(hyperlinks);
    fz_var(head);
    fz_var(tail);

    fz_try(context) {
        annotations = get_page_annotations(context, page->document,
            page->super.number);
        if (annotations != miniexp_nil
            && !is_failed_expression(annotations)) {
            hyperlinks = ddjvu_anno_get_hyperlinks(annotations);
            if (hyperlinks) {
                for (int i = 0; hyperlinks[i] != miniexp_nil; ++i) {
                    miniexp_t map_area = hyperlinks[i];
                    const char* uri = expression_string(
                        miniexp_nth(1, map_area));
                    fz_rect rectangle;
                    if (!uri || !*uri
                        || !hyperlink_rectangle(page,
                            miniexp_nth(3, map_area), &rectangle)) {
                        continue;
                    }

                    char normalized_uri[1024];
                    normalize_uri(page->document, uri, page->super.number,
                        normalized_uri, sizeof(normalized_uri));
                    fz_link* link = fz_new_link_of_size(context,
                        sizeof(fz_link), rectangle, normalized_uri);
                    if (!head) {
                        head = link;
                    }
                    else {
                        tail->next = link;
                    }
                    tail = link;
                }
            }
        }
    }
    fz_always(context) {
        std::free(hyperlinks);
        if (annotations != miniexp_nil) {
            ddjvu_miniexp_release(page->document->document, annotations);
        }
    }
    fz_catch(context) {
        fz_drop_link(context, head);
        fz_rethrow(context);
    }
    return head;
}

fz_page* load_page(fz_context* context, fz_document* document_, int chapter,
    int number) {
    auto* document = reinterpret_cast<DjvuDocument*>(document_);
    if (chapter != 0 || number < 0 || number >= document->page_count) {
        fz_throw(context, FZ_ERROR_ARGUMENT, "Invalid DjVu page %d", number + 1);
    }

    ddjvu_page_t* decoded_page = ddjvu_page_create_by_pageno(
        document->document, number);
    if (!decoded_page) {
        fz_throw(context, FZ_ERROR_FORMAT, "Could not create DjVu page %d",
            number + 1);
    }

    ddjvu_pageinfo_t info = {};
    DjvuPage* page = nullptr;
    fz_var(decoded_page);
    fz_var(page);
    fz_try(context) {
        wait_for_page(context, document, decoded_page);
        info = get_page_info(context, document, number);
        page = fz_new_derived_page(context, DjvuPage, document_);
        page->super.bound_page = bound_page;
        page->super.run_page_contents = run_page;
        page->super.load_links = load_links;
        page->super.drop_page = drop_page;
        page->document = document;
        page->page = decoded_page;
        page->info = info;
        page->pixel_width = ddjvu_page_get_width(decoded_page);
        page->pixel_height = ddjvu_page_get_height(decoded_page);
        page->dpi = std::max(1, ddjvu_page_get_resolution(decoded_page));
        decoded_page = nullptr;
    }
    fz_catch(context) {
        if (decoded_page) {
            ddjvu_page_release(decoded_page);
        }
        fz_rethrow(context);
    }
    return reinterpret_cast<fz_page*>(page);
}

fz_link_dest resolve_link(fz_context*, fz_document* document_,
    const char* uri) {
    auto* document = reinterpret_cast<DjvuDocument*>(document_);
    if (uri && uri[0] == '#') {
        const int page = resolve_page_reference(document, uri + 1, 0);
        if (page >= 0) {
            return fz_make_link_dest_xyz(0, page, 0, 0, 0);
        }
    }
    return fz_make_link_dest_none();
}

void page_label(fz_context*, fz_document* document_, int chapter, int page,
    char* buffer, size_t size) {
    auto* document = reinterpret_cast<DjvuDocument*>(document_);
    if (chapter == 0 && page >= 0 && page < document->page_count
        && document->page_labels && document->page_labels[page]) {
        fz_strlcpy(buffer, document->page_labels[page], size);
    }
    else if (size > 0) {
        buffer[0] = '\0';
    }
}

fz_outline* parse_outline_entries(fz_context* context, DjvuDocument* document,
    miniexp_t entries) {
    fz_outline* head = nullptr;
    fz_outline* tail = nullptr;
    fz_outline* node = nullptr;
    fz_var(head);
    fz_var(tail);
    fz_var(node);

    fz_try(context) {
        for (miniexp_t cursor = entries; miniexp_consp(cursor);
             cursor = miniexp_cdr(cursor)) {
            miniexp_t entry = miniexp_car(cursor);
            if (!miniexp_consp(entry) || miniexp_length(entry) < 2
                || !miniexp_stringp(miniexp_nth(0, entry))) {
                continue;
            }

            const char* title = miniexp_to_str(miniexp_nth(0, entry));
            const char* source_uri = expression_string(miniexp_nth(1, entry));
            char uri[1024] = {};
            normalize_uri(document, source_uri, 0, uri, sizeof(uri));

            node = fz_new_outline(context);
            node->title = fz_strdup(context, title ? title : "");
            node->uri = fz_strdup(context, uri);
            const int target_page = uri[0] == '#'
                ? resolve_page_reference(document, uri + 1, 0)
                : -1;
            node->page = fz_make_location(target_page >= 0 ? 0 : -1,
                target_page);
            node->x = node->y = 0;
            node->is_open = 1;
            node->down = parse_outline_entries(context, document,
                miniexp_cddr(entry));

            if (!head) {
                head = node;
            }
            else {
                tail->next = node;
            }
            tail = node;
            node = nullptr;
        }
    }
    fz_catch(context) {
        fz_drop_outline(context, node);
        fz_drop_outline(context, head);
        fz_rethrow(context);
    }
    return head;
}

fz_outline* load_outline(fz_context* context, fz_document* document_) {
    auto* document = reinterpret_cast<DjvuDocument*>(document_);
    miniexp_t expression = miniexp_nil;
    fz_outline* outline = nullptr;
    fz_var(expression);
    fz_var(outline);
    fz_try(context) {
        expression = get_outline(context, document);
        if (expression != miniexp_nil
            && !is_failed_expression(expression)) {
            miniexp_t entries = is_symbol(miniexp_car(expression), "bookmarks")
                ? miniexp_cdr(expression)
                : expression;
            outline = parse_outline_entries(context, document, entries);
        }
    }
    fz_always(context) {
        if (expression != miniexp_nil) {
            ddjvu_miniexp_release(document->document, expression);
        }
    }
    fz_catch(context) {
        fz_drop_outline(context, outline);
        fz_rethrow(context);
    }
    return outline;
}

int copy_metadata(char* output, size_t size, const char* value) {
    if (!value) {
        return -1;
    }
    return 1 + static_cast<int>(fz_strlcpy(output, value, size));
}

int lookup_metadata(fz_context*, fz_document* document_, const char* key,
    char* output, size_t size) {
    auto* document = reinterpret_cast<DjvuDocument*>(document_);
    if (!std::strcmp(key, FZ_META_FORMAT)) {
        return copy_metadata(output, size, "DjVu");
    }
    if (!std::strcmp(key, FZ_META_INFO_TITLE)) {
        return copy_metadata(output, size, document->title);
    }
    if (!std::strcmp(key, FZ_META_INFO_AUTHOR)) {
        return copy_metadata(output, size, document->author);
    }
    if (!std::strcmp(key, FZ_META_INFO_SUBJECT)) {
        return copy_metadata(output, size, document->subject);
    }
    if (!std::strcmp(key, FZ_META_INFO_CREATOR)) {
        return copy_metadata(output, size, document->creator);
    }
    if (!std::strcmp(key, FZ_META_INFO_PRODUCER)) {
        return copy_metadata(output, size, document->producer);
    }
    if (!std::strcmp(key, FZ_META_INFO_CREATIONDATE)) {
        return copy_metadata(output, size, document->creation_date);
    }
    if (!std::strcmp(key, FZ_META_INFO_MODIFICATIONDATE)) {
        return copy_metadata(output, size, document->modification_date);
    }
    return -1;
}

char* metadata_value(fz_context* context, miniexp_t annotations,
    const char* primary_key, const char* fallback_key = nullptr) {
    const char* value = ddjvu_anno_get_metadata(annotations,
        miniexp_symbol(primary_key));
    if ((!value || !*value) && fallback_key) {
        value = ddjvu_anno_get_metadata(annotations,
            miniexp_symbol(fallback_key));
    }
    return value && *value ? fz_strdup(context, value) : nullptr;
}

void load_metadata(fz_context* context, DjvuDocument* document) {
    miniexp_t annotations = miniexp_nil;
    fz_var(annotations);
    fz_try(context) {
        annotations = get_document_annotations(context, document);
        if (annotations != miniexp_nil
            && !is_failed_expression(annotations)) {
            document->title = metadata_value(context, annotations,
                "Title", "title");
            document->author = metadata_value(context, annotations,
                "Author", "author");
            document->subject = metadata_value(context, annotations,
                "Subject", "subject");
            document->creator = metadata_value(context, annotations,
                "Creator", "creator");
            document->producer = metadata_value(context, annotations,
                "Producer", "producer");
            document->creation_date = metadata_value(context, annotations,
                "CreationDate");
            document->modification_date = metadata_value(context, annotations,
                "ModDate");
        }
    }
    fz_always(context) {
        if (annotations != miniexp_nil) {
            ddjvu_miniexp_release(document->document, annotations);
        }
    }
    fz_catch(context) {
        fz_rethrow(context);
    }
}

void load_page_names(fz_context* context, DjvuDocument* document) {
    document->page_ids = static_cast<char**>(fz_calloc(context,
        document->page_count, sizeof(char*)));
    document->page_labels = static_cast<char**>(fz_calloc(context,
        document->page_count, sizeof(char*)));

    const int file_count = ddjvu_document_get_filenum(document->document);
    for (int file_number = 0; file_number < file_count; ++file_number) {
        ddjvu_fileinfo_t info = {};
        ddjvu_status_t status;
        while ((status = ddjvu_document_get_fileinfo(
                    document->document, file_number, &info)) < DDJVU_JOB_OK) {
            process_messages(context, document, true);
        }
        if (status >= DDJVU_JOB_FAILED || info.pageno < 0
            || info.pageno >= document->page_count) {
            continue;
        }
        if (info.id) {
            document->page_ids[info.pageno] = fz_strdup(context, info.id);
        }
        // The file ID is an internal link target (and is commonly a long
        // generated filename), not a user-facing page label.
        const char* label = info.title;
        const bool is_internal_name = label
            && ((info.id && std::strcmp(label, info.id) == 0)
                || (info.name && std::strcmp(label, info.name) == 0));
        if (label && *label && !is_internal_name) {
            document->page_labels[info.pageno] = fz_strdup(context, label);
        }
    }

    for (int i = 0; i < document->page_count; ++i) {
        if (!document->page_labels[i]) {
            char label[32];
            std::snprintf(label, sizeof(label), "%d", i + 1);
            document->page_labels[i] = fz_strdup(context, label);
        }
    }
}

fz_document* open_document(fz_context* context,
    const fz_document_handler*, fz_stream* stream, fz_stream*, fz_archive* directory,
    void*) {
    auto* document = fz_new_derived_document(context, DjvuDocument);
    document->super.drop_document = drop_document;
    document->super.count_pages = count_pages;
    document->super.load_page = load_page;
    document->super.load_outline = load_outline;
    document->super.resolve_link_dest = resolve_link;
    document->super.page_label = page_label;
    document->super.lookup_metadata = lookup_metadata;

    fz_try(context) {
        fz_seek(context, stream, 0, SEEK_SET);
        document->source = fz_read_all(context, stream, 0);
        document->directory = fz_keep_archive(context, directory);
        document->synchronization = new (std::nothrow) DjvuSynchronization;
        if (!document->synchronization) {
            fz_throw(context, FZ_ERROR_SYSTEM,
                "Could not allocate DjVu synchronization state");
        }
        document->context = ddjvu_context_create("sioyek");
        if (!document->context) {
            fz_throw(context, FZ_ERROR_SYSTEM,
                "Could not initialize DjVuLibre");
        }
        ddjvu_message_set_callback(document->context, notify_djvu_message,
            document->synchronization);
        document->document = ddjvu_document_create(document->context,
            nullptr, FALSE);
        if (!document->document) {
            fz_throw(context, FZ_ERROR_FORMAT,
                "Could not create DjVu decoder");
        }

        size_t size = 0;
        unsigned char* data = nullptr;
        size = fz_buffer_storage(context, document->source, &data);
        ddjvu_stream_write(document->document, 0,
            reinterpret_cast<const char*>(data), size);
        ddjvu_stream_close(document->document, 0, FALSE);
        fz_drop_buffer(context, document->source);
        document->source = nullptr;
        wait_for_document(context, document);

        document->page_count = ddjvu_document_get_pagenum(document->document);
        if (document->page_count <= 0) {
            fz_throw(context, FZ_ERROR_FORMAT, "DjVu document has no pages");
        }
        document->pixel_format = ddjvu_format_create(DDJVU_FORMAT_RGB24, 0,
            nullptr);
        if (!document->pixel_format) {
            fz_throw(context, FZ_ERROR_SYSTEM,
                "Could not create DjVu pixel format");
        }
        ddjvu_format_set_row_order(document->pixel_format, TRUE);
        ddjvu_format_set_y_direction(document->pixel_format, TRUE);
        document->text_font = fz_new_base14_font(context, "Courier");
        load_page_names(context, document);
        load_metadata(context, document);
    }
    fz_catch(context) {
        fz_drop_document(context, reinterpret_cast<fz_document*>(document));
        fz_rethrow(context);
    }
    return reinterpret_cast<fz_document*>(document);
}

int recognize_content(fz_context* context, const fz_document_handler*,
    fz_stream* stream, fz_archive*, void** state,
    fz_document_recognize_state_free_fn** free_state) {
    if (state) {
        *state = nullptr;
    }
    if (free_state) {
        *free_state = nullptr;
    }
    if (!stream) {
        return 0;
    }

    unsigned char signature[8] = {};
    const size_t count = fz_read(context, stream, signature, sizeof(signature));
    return count == sizeof(signature)
            && std::memcmp(signature, "AT&TFORM", sizeof(signature)) == 0
        ? 100
        : 0;
}

const char* extensions[] = { "djvu", "djv", nullptr };
const char* mime_types[] = {
    "image/vnd.djvu",
    "image/x-djvu",
    "application/x-djvu",
    nullptr,
};

fz_document_handler handler = {
    nullptr,
    open_document,
    extensions,
    mime_types,
    recognize_content,
    1,
    0,
    nullptr,
};

} // namespace

void register_djvu_document_handler(fz_context* context) {
    fz_register_document_handler(context, &handler);
}
