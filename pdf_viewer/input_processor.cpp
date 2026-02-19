#include "input_processor.h"
#include "main_widget.h"
#include "ui_manager.h"
#include "utils.h"
#include <qguiapplication.h>
#include <vector>
#include <memory>
#include <algorithm>

extern std::wstring SHIFT_CLICK_COMMAND;
extern std::wstring CONTROL_CLICK_COMMAND;
extern std::wstring ALT_CLICK_COMMAND;
extern std::wstring SHIFT_RIGHT_CLICK_COMMAND;
extern std::wstring CONTROL_RIGHT_CLICK_COMMAND;
extern std::wstring ALT_RIGHT_CLICK_COMMAND;
extern float VERTICAL_MOVE_AMOUNT;
extern float HORIZONTAL_MOVE_AMOUNT;
extern float TOUCHPAD_SENSITIVITY;
extern float ZOOM_INC_FACTOR;
extern bool INVERTED_HORIZONTAL_SCROLLING;
extern bool WHEEL_ZOOM_ON_CURSOR;
extern bool HOVER_OVERVIEW;
extern bool SINGLE_CLICK_SELECTS_WORDS;
extern bool HIGHLIGHT_MIDDLE_CLICK;

InputProcessor::InputProcessor(MainWidget* main_widget) : main_widget(main_widget) {

}

InputProcessor::~InputProcessor() {

}

void InputProcessor::key_event(bool released, QKeyEvent* kevent) {
    main_widget->validate_render();

    if (main_widget->typing_location.has_value()) {

        if (released == false) {
			if (kevent->key() == Qt::Key::Key_Escape) {
				main_widget->handle_escape();
                return;
			}

            bool should_focus = false;
			if (kevent->key() == Qt::Key::Key_Return) {
				main_widget->typing_location.value().next_char();
			}
			else if (kevent->key() == Qt::Key::Key_Backspace) {
                main_widget->typing_location.value().backspace();
			}
			else if (kevent->text().size() > 0) {
				char c = kevent->text().at(0).unicode();
				should_focus = main_widget->typing_location.value().advance(c);
			}

			int page = main_widget->typing_location.value().page;
			fz_rect character_rect = fz_rect_from_quad(main_widget->typing_location.value().character->quad);
			std::optional<fz_rect> wrong_rect = {};

			if (main_widget->typing_location.value().previous_character) {
				wrong_rect = fz_rect_from_quad(main_widget->typing_location.value().previous_character->character->quad);
			}

			if (should_focus) {
				main_widget->main_document_view->set_offset_y(main_widget->typing_location.value().focus_offset());
			}
			main_widget->opengl_widget->set_typing_rect(page, character_rect, wrong_rect);

		}
        return;

    }


    if (released == false) {

        if (kevent->key() == Qt::Key::Key_Escape) {
            main_widget->handle_escape();
        }

        if (kevent->key() == Qt::Key::Key_Return || kevent->key() == Qt::Key::Key_Enter) {
            if (main_widget->ui_manager->is_textbar_visible()) {
                main_widget->ui_manager->hide_textbar();
                main_widget->setFocus();
                main_widget->handle_pending_text_command(main_widget->ui_manager->get_textbar_text());
                return;
            }
        }

        std::vector<int> ignored_codes = {
            Qt::Key::Key_Shift,
            Qt::Key::Key_Control,
            Qt::Key::Key_Alt
        };
        if (std::find(ignored_codes.begin(), ignored_codes.end(), kevent->key()) != ignored_codes.end()) {
            return;
        }
        if (main_widget->is_waiting_for_symbol()) {

            char symb = get_symbol(kevent->key(), kevent->modifiers() & Qt::ShiftModifier, main_widget->pending_command_instance->special_symbols());
            if (symb) {
                main_widget->pending_command_instance->set_symbol_requirement(symb);
                main_widget->advance_command(std::move(main_widget->pending_command_instance));
            }
            return;
        }
        int num_repeats = 0;
        bool is_control_pressed = (kevent->modifiers() & Qt::ControlModifier) || (kevent->modifiers() & Qt::MetaModifier);
        std::vector<std::unique_ptr<Command>> commands = main_widget->input_handler->handle_key(
            kevent,
            kevent->modifiers() & Qt::ShiftModifier,
            is_control_pressed,
            kevent->modifiers() & Qt::AltModifier,
            &num_repeats);

        for (auto& command : commands) {
            main_widget->handle_command_types(std::move(command), num_repeats);
        }
    }

}

void InputProcessor::handle_right_click(WindowPos click_pos, bool down, bool is_shift_pressed, bool is_control_pressed, bool is_alt_pressed) {

    if (main_widget->is_rotated()) {
        return;
    }
    if (is_shift_pressed || is_control_pressed || is_alt_pressed) {
        return;
    }

    if ((down == true) && main_widget->opengl_widget->get_overview_page()) {
        main_widget->opengl_widget->set_overview_page({});
        //main_document_view->set_line_index(-1);
        main_widget->invalidate_render();
        return;
    }

    if ((main_widget->main_document_view->get_document() != nullptr) && (main_widget->opengl_widget != nullptr)) {

        // disable visual mark and overview window when we are in synctex mode
        // because we probably don't need them (we are editing our own document after all)
        // we can always use middle click to jump to a destination which is probably what we
        // need anyway
        if (down == true && (!main_widget->synctex_mode)) {
            if (main_widget->pending_command_instance && (main_widget->pending_command_instance->get_name() == "goto_mark")) {
                main_widget->return_to_last_visual_mark();
                return;
            }

            if (main_widget->overview_under_pos(click_pos)) {
                return;
            }

            main_widget->visual_mark_under_pos(click_pos);

        }
        else {
            if (main_widget->synctex_mode) {
                if (down == false) {
					main_widget->synctex_under_pos(click_pos);
                }
            }
        }

    }

}

void InputProcessor::handle_left_click(WindowPos click_pos, bool down, bool is_shift_pressed, bool is_control_pressed, bool is_alt_pressed) {

    if (main_widget->is_rotated()) {
        return;
    }
    if (is_shift_pressed || is_control_pressed || is_alt_pressed) {
        return;
    }

    AbsoluteDocumentPos abs_doc_pos = main_widget->main_document_view->window_to_absolute_document_pos(click_pos);

    auto [normal_x, normal_y] = main_widget->main_document_view->window_to_normalized_window_pos(click_pos);

    if (main_widget->opengl_widget) main_widget->opengl_widget->set_should_draw_vertical_line(false);

    if (main_widget->rect_select_mode) {
        if (down == true) {
            if (main_widget->rect_select_end.has_value()) {
                //clicked again after selecting, we should clear the selected rectangle
                main_widget->clear_selected_rect();
            }
            else {
                main_widget->rect_select_begin = abs_doc_pos;
            }
        }
        else {
            if (main_widget->rect_select_begin.has_value() && main_widget->rect_select_end.has_value()) {
				main_widget->rect_select_end = abs_doc_pos;
				fz_rect selected_rectangle;
				selected_rectangle.x0 = main_widget->rect_select_begin.value().x;
				selected_rectangle.y0 = main_widget->rect_select_begin.value().y;
				selected_rectangle.x1 = main_widget->rect_select_end.value().x;
				selected_rectangle.y1 = main_widget->rect_select_end.value().y;
				main_widget->opengl_widget->set_selected_rectangle(selected_rectangle);

                // is pending rect command
                if (main_widget->pending_command_instance) {
                    main_widget->pending_command_instance->set_rect_requirement(selected_rectangle);
                    main_widget->advance_command(std::move(main_widget->pending_command_instance));
                }

				main_widget->rect_select_mode = false;
				main_widget->rect_select_begin = {};
				main_widget->rect_select_end = {};
            }

        }
		return;
    }
    else {
        if (down == true) {
            main_widget->clear_selected_rect();
        }
    }

    if (down == true) {

        PdfViewOpenGLWidget::OverviewSide border_index = static_cast<PdfViewOpenGLWidget::OverviewSide>(-1);
        if (main_widget->opengl_widget->is_window_point_in_overview_border(normal_x, normal_y, &border_index)) {
            PdfViewOpenGLWidget::OverviewResizeData resize_data;
            resize_data.original_normal_mouse_pos = NormalizedWindowPos{ normal_x, normal_y };
            resize_data.original_rect = main_widget->opengl_widget->get_overview_rect();
            resize_data.side_index = border_index;
            main_widget->overview_resize_data = resize_data;
            return;
        }
        if (main_widget->opengl_widget->is_window_point_in_overview({ normal_x, normal_y })) {
            float original_offset_x, original_offset_y;

            PdfViewOpenGLWidget::OverviewMoveData move_data;
            main_widget->opengl_widget->get_overview_offsets(&original_offset_x, &original_offset_y);
            move_data.original_normal_mouse_pos = NormalizedWindowPos{ normal_x, normal_y };
            move_data.original_offsets = fvec2{ original_offset_x, original_offset_y };
            main_widget->overview_move_data = move_data;
            return;
        }

        main_widget->selection_begin = abs_doc_pos;

        main_widget->last_mouse_down = abs_doc_pos;
        main_widget->last_mouse_down_window_pos = click_pos;
        main_widget->last_mouse_down_document_offset = main_widget->main_document_view->get_offsets();

        main_widget->main_document_view->selected_character_rects.clear();

        if (!main_widget->mouse_drag_mode) {
            main_widget->is_selecting = true;
			if (SINGLE_CLICK_SELECTS_WORDS) {
				main_widget->is_word_selecting = true;
			}
        }
        else {
            main_widget->is_dragging = true;
        }
    }
    else {
        main_widget->selection_end = abs_doc_pos;

        main_widget->is_selecting = false;
        main_widget->is_dragging = false;

        bool was_overview_mode = main_widget->overview_move_data.has_value() || main_widget->overview_resize_data.has_value();

        main_widget->overview_move_data = {};
        main_widget->overview_resize_data = {};

        if ((!was_overview_mode) && (!main_widget->mouse_drag_mode) && (manhattan_distance(fvec2(main_widget->last_mouse_down), fvec2(abs_doc_pos)) > 5)) {

            main_widget->main_document_view->get_text_selection(main_widget->last_mouse_down,
                abs_doc_pos,
                main_widget->is_word_selecting,
                main_widget->main_document_view->selected_character_rects,
                main_widget->selected_text);
            main_widget->is_word_selecting = false;
        }
        else {
            main_widget->handle_click(click_pos);
            main_widget->clear_selected_text();
        }
        main_widget->validate_render();
    }
}

void InputProcessor::mouseMoveEvent(QMouseEvent* mouse_event) {

    if (main_widget->is_rotated()) {
        // we don't handle mouse events while document is rotated becausae proper handling
        // would increase the code complexity too much to be worth it
        return;
    }

    WindowPos mpos = { mouse_event->pos().x(), mouse_event->pos().y() };

    std::optional<PdfLink> link = {};

    NormalizedWindowPos normal_mpos = main_widget->main_document_view->window_to_normalized_window_pos(mpos);

    if (main_widget->rect_select_mode) {
        if (main_widget->rect_select_begin.has_value()) {
			AbsoluteDocumentPos abspos = main_widget->main_document_view->window_to_absolute_document_pos(mpos);
			main_widget->rect_select_end = abspos;
			fz_rect selected_rect;
			selected_rect.x0 = main_widget->rect_select_begin.value().x;
			selected_rect.y0 = main_widget->rect_select_begin.value().y;
			selected_rect.x1 = main_widget->rect_select_end.value().x;
			selected_rect.y1 = main_widget->rect_select_end.value().y;
			main_widget->opengl_widget->set_selected_rectangle(selected_rect);

			main_widget->validate_render();
        }
        return;
    }

    if (main_widget->overview_resize_data) {
        // if we are resizing overview page, set the selected side of the overview window to the mosue position
        fvec2 offset_diff = fvec2(normal_mpos) - fvec2(main_widget->overview_resize_data.value().original_normal_mouse_pos);
        main_widget->opengl_widget->set_overview_side_pos(
            main_widget->overview_resize_data.value().side_index,
            main_widget->overview_resize_data.value().original_rect,
            offset_diff);
        main_widget->validate_render();
        return;
    }

    if (main_widget->overview_move_data) {
        fvec2 offset_diff = fvec2(normal_mpos) - fvec2(main_widget->overview_move_data.value().original_normal_mouse_pos);
        offset_diff[1] = -offset_diff[1];
        fvec2 new_offsets = main_widget->overview_move_data.value().original_offsets + offset_diff;
        main_widget->opengl_widget->set_overview_offsets(new_offsets);
        main_widget->validate_render();
        return;
    }

    if (main_widget->opengl_widget->is_window_point_in_overview(normal_mpos)) {
        link = main_widget->doc()->get_link_in_pos(main_widget->opengl_widget->window_pos_to_overview_pos(normal_mpos));
        if (link) {
			main_widget->setCursor(Qt::PointingHandCursor);
        }
        else {
			main_widget->setCursor(Qt::ArrowCursor);
        }
        return;
    }

    if (main_widget->main_document_view && (link = main_widget->main_document_view->get_link_in_pos(mpos))) {
        // show hand cursor when hovering over links
        main_widget->setCursor(Qt::PointingHandCursor);

        // if hover_overview config is set, we show an overview of links while hovering over them
        if (HOVER_OVERVIEW) {
            main_widget->set_overview_link(link.value());
        }
    }
    else {
        main_widget->setCursor(Qt::ArrowCursor);
        if (HOVER_OVERVIEW) {
            main_widget->opengl_widget->set_overview_page({});
            main_widget->invalidate_render();
        }
    }

    if (main_widget->is_dragging) {
        ivec2 diff = ivec2(mpos) - ivec2(main_widget->last_mouse_down_window_pos);

        fvec2 diff_doc = diff / main_widget->main_document_view->get_zoom_level();
        if (main_widget->horizontal_scroll_locked) {
            diff_doc.values[0] = 0;
        }

        main_widget->main_document_view->set_offsets(main_widget->last_mouse_down_document_offset.x + diff_doc.x(),
            main_widget->last_mouse_down_document_offset.y - diff_doc.y());
        main_widget->validate_render();
    }

    if (main_widget->is_selecting) {

        // When selecting, we occasionally update selected text
        //todo: maybe have a timer event that handles this periodically
	int msecs_since_last_text_select = main_widget->last_text_select_time.msecsTo(QTime::currentTime());
	if (msecs_since_last_text_select > 16 || msecs_since_last_text_select < 0) {

            AbsoluteDocumentPos document_pos = main_widget->main_document_view->window_to_absolute_document_pos(mpos);

            main_widget->selection_begin = main_widget->last_mouse_down;
            main_widget->selection_end = document_pos;

            main_widget->main_document_view->get_text_selection(main_widget->selection_begin,
                main_widget->selection_end,
                main_widget->is_word_selecting,
                main_widget->main_document_view->selected_character_rects,
                main_widget->selected_text);

            main_widget->validate_render();
            main_widget->last_text_select_time = QTime::currentTime();
        }
    }

}

void InputProcessor::mousePressEvent(QMouseEvent* mevent) {
    bool is_shift_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier);
    bool is_control_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ControlModifier);
    bool is_alt_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::AltModifier);

    if (mevent->button() == Qt::MouseButton::LeftButton) {
        handle_left_click({ mevent->pos().x(), mevent->pos().y() }, true, is_shift_pressed, is_control_pressed, is_alt_pressed);
    }

    if (mevent->button() == Qt::MouseButton::RightButton) {
        handle_right_click({ mevent->pos().x(), mevent->pos().y() }, true, is_shift_pressed, is_control_pressed, is_alt_pressed);
    }

    if (mevent->button() == Qt::MouseButton::XButton1) {
        main_widget->handle_command_types(main_widget->command_manager->get_command_with_name("prev_state"), 0);
        main_widget->invalidate_render();
    }

    if (mevent->button() == Qt::MouseButton::XButton2) {
        main_widget->handle_command_types(main_widget->command_manager->get_command_with_name("next_state"), 0);
        main_widget->invalidate_render();
    }
}

void InputProcessor::mouseReleaseEvent(QMouseEvent* mevent) {

    bool is_shift_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier);
    bool is_control_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ControlModifier);
    bool is_alt_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::AltModifier);

	if (main_widget->is_rotated()) {
		return;
	}

    if (mevent->button() == Qt::MouseButton::LeftButton) {
        if (is_shift_pressed) {
			auto commands = main_widget->command_manager->create_macro_command("", SHIFT_CLICK_COMMAND);
			commands->run(main_widget);
        }
        else if (is_control_pressed) {
			auto commands = main_widget->command_manager->create_macro_command("", CONTROL_CLICK_COMMAND);
			commands->run(main_widget);
        }
        else if (is_alt_pressed) {
			auto commands = main_widget->command_manager->create_macro_command("", ALT_CLICK_COMMAND);
			commands->run(main_widget);
        }
        else {
			handle_left_click({ mevent->pos().x(), mevent->pos().y() }, false, is_shift_pressed, is_control_pressed, is_alt_pressed);
			if (main_widget->is_select_highlight_mode && (main_widget->main_document_view->selected_character_rects.size() > 0)) {
				main_widget->main_document_view->add_highlight(main_widget->selection_begin, main_widget->selection_end, main_widget->select_highlight_type);
                main_widget->clear_selected_text();
			}
			if (main_widget->main_document_view->selected_character_rects.size() > 0) {
				copy_to_clipboard(main_widget->selected_text, true);
			}
        }

    }

    if (mevent->button() == Qt::MouseButton::RightButton) {
        if (is_shift_pressed) {
			auto commands = main_widget->command_manager->create_macro_command("", SHIFT_RIGHT_CLICK_COMMAND);
			commands->run(main_widget);
        }
        else if (is_control_pressed) {
			auto commands = main_widget->command_manager->create_macro_command("", CONTROL_RIGHT_CLICK_COMMAND);
			commands->run(main_widget);
        }
        else if (is_alt_pressed) {
			auto commands = main_widget->command_manager->create_macro_command("", ALT_RIGHT_CLICK_COMMAND);
			commands->run(main_widget);
        }
        else {
			handle_right_click({ mevent->pos().x(), mevent->pos().y() }, false, is_shift_pressed, is_control_pressed, is_alt_pressed);
        }
    }

    if (mevent->button() == Qt::MouseButton::MiddleButton) {
        if (HIGHLIGHT_MIDDLE_CLICK
            && main_widget->main_document_view->selected_character_rects.size() > 0
            && !(main_widget->opengl_widget && main_widget->opengl_widget->get_overview_page())) {
            main_widget->command_manager->get_command_with_name("add_highlight_with_current_type")->run(main_widget);
            main_widget->invalidate_render();
        }
        else {
          main_widget->smart_jump_under_pos({ mevent->pos().x(), mevent->pos().y() });
        }
    }

}

void InputProcessor::mouseDoubleClickEvent(QMouseEvent* mevent) {
	if (mevent->button() == Qt::MouseButton::LeftButton) {
		main_widget->is_selecting = true;
		if (SINGLE_CLICK_SELECTS_WORDS) {
			main_widget->is_word_selecting = false;
		}
        else {
			main_widget->is_word_selecting = true;
		}
	}
}

void InputProcessor::wheelEvent(QWheelEvent* wevent) {

    std::unique_ptr<Command> command = nullptr;
    float vertical_move_amount = VERTICAL_MOVE_AMOUNT * TOUCHPAD_SENSITIVITY;
    float horizontal_move_amount = HORIZONTAL_MOVE_AMOUNT * TOUCHPAD_SENSITIVITY;

    if (main_widget->main_document_view_has_document()) {
        main_widget->main_document_view->disable_auto_resize_mode();
    }

    bool is_control_pressed = QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier) ||
        QGuiApplication::queryKeyboardModifiers().testFlag(Qt::MetaModifier);

    bool is_shift_pressed = QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier);
    bool is_visual_mark_mode = main_widget->opengl_widget->get_should_draw_vertical_line() && main_widget->visual_scroll_mode;


#ifdef SIOYEK_QT6
    int x = wevent->position().x();
    int y = wevent->position().y();
#else
    int x = wevent->pos().x();
    int y = wevent->pos().y();
#endif

    WindowPos mouse_window_pos = { x, y };
    auto [normal_x, normal_y] = main_widget->main_document_view->window_to_normalized_window_pos(mouse_window_pos);

#ifdef SIOYEK_QT6
	int num_repeats = abs(wevent->angleDelta().y() / 120);
	float num_repeats_f = abs(wevent->angleDelta().y() / 120.0);
#else
	int num_repeats = abs(wevent->delta() / 120);
	float num_repeats_f = abs(wevent->delta() / 120.0);
#endif

    if (num_repeats == 0) {
        num_repeats = 1;
    }

    if ((!is_control_pressed) && (!is_shift_pressed)) {
        if (main_widget->opengl_widget->is_window_point_in_overview({ normal_x, normal_y })) {
            if (wevent->angleDelta().y() > 0) {
                main_widget->scroll_overview(-1);
            }
            if (wevent->angleDelta().y() < 0) {
                main_widget->scroll_overview(1);
            }
            main_widget->validate_render();
        }
        else {

            if (wevent->angleDelta().y() > 0) {

                if (is_visual_mark_mode) {
                    command = main_widget->command_manager->get_command_with_name("move_visual_mark_up");
                }
                else {
                    main_widget->move_vertical(-72.0f * vertical_move_amount * num_repeats_f);
					main_widget->update_scrollbar();
                    return;
                }
            }
            if (wevent->angleDelta().y() < 0) {

                if (is_visual_mark_mode) {
                    command = main_widget->command_manager->get_command_with_name("move_visual_mark_down");
                }
                else {
                    main_widget->move_vertical(72.0f * vertical_move_amount * num_repeats_f);
					main_widget->update_scrollbar();
                    return;
                }
            }

			float inverse_factor = INVERTED_HORIZONTAL_SCROLLING ? -1.0f : 1.0f;

            if (wevent->angleDelta().x() > 0) {
                main_widget->move_horizontal(-72.0f * horizontal_move_amount * num_repeats_f * inverse_factor);
                return;
            }
            if (wevent->angleDelta().x() < 0) {
                main_widget->move_horizontal(72.0f * horizontal_move_amount * num_repeats_f * inverse_factor);
                return;
            }
        }
    }

    if (is_control_pressed) {
        float zoom_factor = 1.0f + num_repeats_f * (ZOOM_INC_FACTOR - 1.0f);
        main_widget->zoom(mouse_window_pos, zoom_factor, wevent->angleDelta().y() > 0);
        return;
    }
    if (is_shift_pressed) {
        float inverse_factor = INVERTED_HORIZONTAL_SCROLLING ? -1.0f : 1.0f;

        if (wevent->angleDelta().y() > 0) {
            main_widget->move_horizontal(-72.0f * horizontal_move_amount * num_repeats_f * inverse_factor);
            return;
        }
        if (wevent->angleDelta().y() < 0) {
            main_widget->move_horizontal(72.0f * horizontal_move_amount * num_repeats_f * inverse_factor);
            return;
        }

    }

    if (command) {
        command->set_num_repeats(num_repeats);
        command->run(main_widget);
    }
}
