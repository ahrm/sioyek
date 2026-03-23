#include "mark_parser.h"
#include <cstring>
#include <iostream>
#include <qpainter.h>
#include <qregularexpression.h>

MarkFileParser::MarkFileParser() {}

MarkFileParser::~MarkFileParser() {}

uint32_t MarkFileParser::read_uint32_le(QFile& file) {
    uint8_t bytes[4];
    if (file.read(reinterpret_cast<char*>(bytes), 4) != 4) {
        return 0;
    }
    return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
}

QByteArray MarkFileParser::read_data_at_address(QFile& file, uint32_t address) {
    if (address == 0) {
        return QByteArray();
    }

    if (!file.seek(address)) {
        return QByteArray();
    }

    uint32_t length = read_uint32_le(file);
    if (length == 0 || length > 100000000) {  // Sanity check
        return QByteArray();
    }

    return file.read(length);
}

QMap<QString, QVariant> MarkFileParser::parse_key_value_pairs(const QByteArray& data) {
    QMap<QString, QVariant> result;
    QString str = QString::fromUtf8(data);

    // Parse <KEY:VALUE> pairs using regex
    QRegularExpression regex("<([^:<>]+):([^:<>]*)>");
    QRegularExpressionMatchIterator it = regex.globalMatch(str);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString key = match.captured(1);
        QString value = match.captured(2);

        // Handle duplicate keys by storing as list
        if (result.contains(key)) {
            QVariant existing = result[key];
            if (existing.type() == QVariant::StringList) {
                QStringList list = existing.toStringList();
                list.append(value);
                result[key] = list;
            } else {
                QStringList list;
                list.append(existing.toString());
                list.append(value);
                result[key] = list;
            }
        } else {
            result[key] = value;
        }
    }

    return result;
}

QMap<QString, QVariant> MarkFileParser::parse_metadata_block(QFile& file, uint32_t address) {
    QByteArray data = read_data_at_address(file, address);
    if (data.isEmpty()) {
        return QMap<QString, QVariant>();
    }
    return parse_key_value_pairs(data);
}

bool MarkFileParser::load(const QString& mark_file_path) {
    file_path_ = mark_file_path;
    file_data_ = MarkFileData();
    cached_pixmaps_.clear();

    QFile file(mark_file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        std::cerr << "[sioyek] MarkFileParser: failed to open " << mark_file_path.toStdString() << std::endl;
        return false;
    }

    // Check file type (first 4 bytes) - can be "mark" or "MARK"
    QByteArray file_type = file.read(4);
    if (file_type.toLower() != "mark") {
        std::cerr << "[sioyek] MarkFileParser: not a MARK file (got: " << file_type.toStdString() << ")" << std::endl;
        return false;
    }
    file_data_.file_type = QString::fromLatin1(file_type).toUpper();

    // Read footer address from last 4 bytes
    file.seek(file.size() - 4);
    uint32_t footer_address = read_uint32_le(file);

    if (footer_address == 0 || footer_address >= file.size()) {
        std::cerr << "[sioyek] MarkFileParser: invalid footer address" << std::endl;
        return false;
    }

    // Parse footer block
    QMap<QString, QVariant> footer = parse_metadata_block(file, footer_address);
    if (footer.isEmpty()) {
        std::cerr << "[sioyek] MarkFileParser: failed to parse footer" << std::endl;
        return false;
    }

    // Parse header block (FILE_FEATURE points to it)
    if (footer.contains("FILE_FEATURE")) {
        uint32_t header_address = footer["FILE_FEATURE"].toString().toUInt();
        QMap<QString, QVariant> header = parse_metadata_block(file, header_address);

        if (header.contains("APPLY_EQUIPMENT")) {
            file_data_.device_model = header["APPLY_EQUIPMENT"].toString();
            // Check for X2 device with different resolution
            if (file_data_.device_model.contains("X2") || file_data_.device_model == "N5") {
                file_data_.width = 1920;
                file_data_.height = 2560;
            }
        }
    }

    // Parse pages (PAGE1, PAGE2, etc.)
    // Note: Pages may be non-sequential (e.g., PAGE1, PAGE3 without PAGE2)
    // because .mark files only store pages that have annotations.
    // We iterate through all keys in footer that start with "PAGE".
    for (auto it = footer.constBegin(); it != footer.constEnd(); ++it) {
        QString key = it.key();
        if (!key.startsWith("PAGE")) {
            continue;
        }

        // Extract page number from key (e.g., "PAGE3" -> 3)
        bool ok;
        int page_num_1indexed = key.mid(4).toInt(&ok);
        if (!ok || page_num_1indexed < 1) {
            continue;
        }
        int page_num = page_num_1indexed - 1;  // Convert to 0-indexed

        uint32_t page_address = it.value().toString().toUInt();
        if (page_address == 0) {
            continue;
        }

        QMap<QString, QVariant> page_meta = parse_metadata_block(file, page_address);
        if (page_meta.isEmpty()) {
            continue;
        }

        MarkPageInfo page_info;
        page_info.page_number = page_num;

        if (page_meta.contains("ORIENTATION")) {
            page_info.orientation = page_meta["ORIENTATION"].toString();
        }

        // Get main layer address
        if (page_meta.contains("MAINLAYER")) {
            page_info.main_layer_address = page_meta["MAINLAYER"].toString().toUInt();

            if (page_info.main_layer_address > 0) {
                // Parse layer metadata
                QMap<QString, QVariant> layer_meta = parse_metadata_block(file, page_info.main_layer_address);

                MarkLayerInfo layer;
                layer.name = "MAINLAYER";
                if (layer_meta.contains("LAYERPROTOCOL")) {
                    layer.protocol = layer_meta["LAYERPROTOCOL"].toString();
                }
                if (layer_meta.contains("LAYERBITMAP")) {
                    layer.bitmap_address = layer_meta["LAYERBITMAP"].toString().toUInt();
                }
                if (layer_meta.contains("LAYERTYPE")) {
                    layer.type = layer_meta["LAYERTYPE"].toString();
                }
                layer.is_visible = true;
                page_info.layers.push_back(layer);
            }
        }

        // Also check other layers (LAYER1, LAYER2, LAYER3, BGLAYER)
        QStringList layer_keys = {"LAYER1", "LAYER2", "LAYER3", "BGLAYER"};
        for (const QString& layer_key : layer_keys) {
            if (page_meta.contains(layer_key)) {
                uint32_t layer_address = page_meta[layer_key].toString().toUInt();
                if (layer_address > 0) {
                    QMap<QString, QVariant> layer_meta = parse_metadata_block(file, layer_address);

                    MarkLayerInfo layer;
                    layer.name = layer_key;
                    if (layer_meta.contains("LAYERPROTOCOL")) {
                        layer.protocol = layer_meta["LAYERPROTOCOL"].toString();
                    }
                    if (layer_meta.contains("LAYERBITMAP")) {
                        layer.bitmap_address = layer_meta["LAYERBITMAP"].toString().toUInt();
                    }
                    if (layer_meta.contains("LAYERTYPE")) {
                        layer.type = layer_meta["LAYERTYPE"].toString();
                    }
                    // Background layers are typically not for annotations
                    layer.is_visible = (layer_key != "BGLAYER");
                    page_info.layers.push_back(layer);
                }
            }
        }

        file_data_.pages.push_back(page_info);
    }

    file_data_.is_valid = true;
    std::cerr << "[sioyek] MarkFileParser: loaded " << file_data_.pages.size()
              << " pages from " << mark_file_path.toStdString() << std::endl;

    return true;
}

uint32_t MarkFileParser::color_code_to_rgb(uint8_t color_code, bool is_x2_device) {
    switch (color_code) {
        case MarkColorCodes::BLACK:
        case MarkColorCodes::MARKER_BLACK:
            return MarkPalette::BLACK;

        case MarkColorCodes::BACKGROUND:
            return MarkPalette::TRANSPARENT;

        case MarkColorCodes::DARK_GRAY:
        case MarkColorCodes::MARKER_DARK_GRAY:
            return MarkPalette::DARK_GRAY;

        case MarkColorCodes::DARK_GRAY_X2:
        case MarkColorCodes::MARKER_DARK_GRAY_X2:
            return MarkPalette::DARK_GRAY;

        case MarkColorCodes::GRAY:
        case MarkColorCodes::MARKER_GRAY:
            return MarkPalette::GRAY;

        case MarkColorCodes::GRAY_X2:
        case MarkColorCodes::MARKER_GRAY_X2:
            return MarkPalette::GRAY;

        case MarkColorCodes::WHITE:
            return MarkPalette::WHITE;

        default:
            return MarkPalette::TRANSPARENT;
    }
}

QByteArray MarkFileParser::decode_rle(const QByteArray& compressed_data, int expected_size, bool is_x2_device) {
    QByteArray result;
    result.reserve(expected_size);

    int pos = 0;
    int data_len = compressed_data.size();

    // State for handling high-bit continuation
    bool has_holder = false;
    uint8_t holder_color = 0;
    uint8_t holder_length = 0;

    while (pos + 1 < data_len && result.size() < expected_size) {
        uint8_t color_code = static_cast<uint8_t>(compressed_data[pos]);
        uint8_t length_byte = static_cast<uint8_t>(compressed_data[pos + 1]);
        pos += 2;

        int run_length = 0;

        if (has_holder) {
            if (color_code == holder_color) {
                // Combine with holder
                run_length = 1 + length_byte + (((holder_length & 0x7f) + 1) << 7);
            } else {
                // Output holder first, then process current
                int holder_run = ((holder_length & 0x7f) + 1) << 7;
                uint32_t rgb = color_code_to_rgb(holder_color, is_x2_device);

                for (int i = 0; i < holder_run && result.size() < expected_size; i++) {
                    result.append(static_cast<char>((rgb >> 16) & 0xff));  // R
                    result.append(static_cast<char>((rgb >> 8) & 0xff));   // G
                    result.append(static_cast<char>(rgb & 0xff));          // B
                    result.append(static_cast<char>(rgb == MarkPalette::TRANSPARENT ? 0x00 : 0xff));  // A
                }

                // Now process current pair
                if (length_byte == MarkColorCodes::SPECIAL_LENGTH_MARKER) {
                    run_length = MarkColorCodes::SPECIAL_LENGTH;
                } else if (length_byte & 0x80) {
                    // New holder
                    holder_color = color_code;
                    holder_length = length_byte;
                    has_holder = true;
                    continue;
                } else {
                    run_length = length_byte + 1;
                }
            }
            has_holder = false;
        } else {
            if (length_byte == MarkColorCodes::SPECIAL_LENGTH_MARKER) {
                run_length = MarkColorCodes::SPECIAL_LENGTH;
            } else if (length_byte & 0x80) {
                // Set holder for next iteration
                holder_color = color_code;
                holder_length = length_byte;
                has_holder = true;
                continue;
            } else {
                run_length = length_byte + 1;
            }
        }

        // Output the run
        uint32_t rgb = color_code_to_rgb(color_code, is_x2_device);
        uint8_t alpha = (rgb == MarkPalette::TRANSPARENT) ? 0x00 : 0xff;

        for (int i = 0; i < run_length && result.size() < expected_size; i++) {
            result.append(static_cast<char>((rgb >> 16) & 0xff));  // R
            result.append(static_cast<char>((rgb >> 8) & 0xff));   // G
            result.append(static_cast<char>(rgb & 0xff));          // B
            result.append(static_cast<char>(alpha));               // A
        }
    }

    // Handle remaining holder
    if (has_holder && result.size() < expected_size) {
        int remaining = (expected_size - result.size()) / 4;
        uint32_t rgb = color_code_to_rgb(holder_color, is_x2_device);
        uint8_t alpha = (rgb == MarkPalette::TRANSPARENT) ? 0x00 : 0xff;

        for (int i = 0; i < remaining; i++) {
            result.append(static_cast<char>((rgb >> 16) & 0xff));
            result.append(static_cast<char>((rgb >> 8) & 0xff));
            result.append(static_cast<char>(rgb & 0xff));
            result.append(static_cast<char>(alpha));
        }
    }

    // Pad with transparent pixels if needed
    while (result.size() < expected_size) {
        result.append('\xff');  // R
        result.append('\xff');  // G
        result.append('\xff');  // B
        result.append('\x00');  // A (transparent)
    }

    return result;
}

QImage MarkFileParser::bitmap_to_image(const QByteArray& bitmap_data, int width, int height) {
    if (bitmap_data.size() < width * height * 4) {
        std::cerr << "[sioyek] MarkFileParser: bitmap data too small" << std::endl;
        return QImage();
    }

    QImage image(width, height, QImage::Format_RGBA8888);

    const uchar* src = reinterpret_cast<const uchar*>(bitmap_data.constData());
    for (int y = 0; y < height; y++) {
        uchar* dest = image.scanLine(y);
        memcpy(dest, src + y * width * 4, width * 4);
    }

    return image;
}

int MarkFileParser::page_count() const {
    return static_cast<int>(file_data_.pages.size());
}

bool MarkFileParser::has_page(int page) const {
    for (const auto& p : file_data_.pages) {
        if (p.page_number == page) {
            return true;
        }
    }
    return false;
}

QPixmap MarkFileParser::get_page_pixmap(int page) {
    // Check cache first
    auto it = cached_pixmaps_.find(page);
    if (it != cached_pixmaps_.end()) {
        return it->second;
    }

    // Find the page
    const MarkPageInfo* page_info = nullptr;
    for (const auto& p : file_data_.pages) {
        if (p.page_number == page) {
            page_info = &p;
            break;
        }
    }

    if (!page_info) {
        return QPixmap();
    }

    // Open file for reading bitmap data
    QFile file(file_path_);
    if (!file.open(QIODevice::ReadOnly)) {
        return QPixmap();
    }

    bool is_x2 = file_data_.device_model.contains("X2") || file_data_.device_model == "N5";
    int width = file_data_.width;
    int height = file_data_.height;

    // Handle orientation
    if (page_info->orientation == "1090") {
        std::swap(width, height);
    }

    int expected_size = width * height * 4;  // RGBA

    // Decode layers and composite them
    QImage final_image(width, height, QImage::Format_RGBA8888);
    final_image.fill(Qt::transparent);

    for (const auto& layer : page_info->layers) {
        if (!layer.is_visible || layer.bitmap_address == 0) {
            continue;
        }

        QByteArray compressed = read_data_at_address(file, layer.bitmap_address);
        if (compressed.isEmpty()) {
            continue;
        }

        QByteArray decoded = decode_rle(compressed, expected_size, is_x2);
        if (decoded.isEmpty()) {
            continue;
        }

        QImage layer_image = bitmap_to_image(decoded, width, height);
        if (layer_image.isNull()) {
            continue;
        }

        // Composite layer onto final image
        QPainter painter(&final_image);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawImage(0, 0, layer_image);
        painter.end();
    }

    QPixmap pixmap = QPixmap::fromImage(final_image);
    cached_pixmaps_[page] = pixmap;

    return pixmap;
}
