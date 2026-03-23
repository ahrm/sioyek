#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <vector>
#include <qfile.h>
#include <qimage.h>
#include <qmap.h>
#include <qpixmap.h>
#include <qstring.h>

// Color codes used in Supernote RLE encoding
namespace MarkColorCodes {
    constexpr uint8_t BLACK = 0x61;
    constexpr uint8_t BACKGROUND = 0x62;
    constexpr uint8_t DARK_GRAY = 0x63;
    constexpr uint8_t GRAY = 0x64;
    constexpr uint8_t WHITE = 0x65;
    constexpr uint8_t MARKER_BLACK = 0x66;
    constexpr uint8_t MARKER_DARK_GRAY = 0x67;
    constexpr uint8_t MARKER_GRAY = 0x68;

    // X2 device color codes
    constexpr uint8_t DARK_GRAY_X2 = 0x9D;
    constexpr uint8_t GRAY_X2 = 0xC9;
    constexpr uint8_t MARKER_DARK_GRAY_X2 = 0x9E;
    constexpr uint8_t MARKER_GRAY_X2 = 0xCA;

    // Special markers
    constexpr uint8_t SPECIAL_LENGTH_MARKER = 0xff;
    constexpr int SPECIAL_LENGTH = 0x4000;
    constexpr int SPECIAL_LENGTH_FOR_BLANK = 0x400;
}

// RGB color palette values
namespace MarkPalette {
    constexpr uint32_t BLACK = 0x000000;
    constexpr uint32_t DARK_GRAY = 0x9d9d9d;
    constexpr uint32_t GRAY = 0xc9c9c9;
    constexpr uint32_t WHITE = 0xfefefe;
    constexpr uint32_t TRANSPARENT = 0xffffff;
}

// Metadata for a layer within a page
struct MarkLayerInfo {
    QString name;
    QString type;
    QString protocol;
    uint32_t bitmap_address = 0;
    bool is_visible = true;
};

// Metadata for a page
struct MarkPageInfo {
    int page_number = 0;
    QString orientation;  // "1000" = portrait, "1090" = horizontal
    std::vector<MarkLayerInfo> layers;
};

// Parsed .mark file
struct MarkFileData {
    QString file_type;
    QString device_model;
    int width = 1404;
    int height = 1872;
    std::vector<MarkPageInfo> pages;
    bool is_valid = false;
};

class MarkFileParser {
public:
    MarkFileParser();
    ~MarkFileParser();

    // Load and parse a .mark file
    bool load(const QString& mark_file_path);

    // Get the number of pages with annotations
    int page_count() const;

    // Get the decoded bitmap as a QPixmap for a specific page
    // Returns null QPixmap if page doesn't exist or decoding fails
    QPixmap get_page_pixmap(int page);

    // Get page dimensions
    int get_width() const { return file_data_.width; }
    int get_height() const { return file_data_.height; }

private:
    // Parse metadata block at given address
    QMap<QString, QVariant> parse_metadata_block(QFile& file, uint32_t address);

    // Parse key-value pairs from metadata string
    QMap<QString, QVariant> parse_key_value_pairs(const QByteArray& data);

    // Read a 4-byte little-endian integer from file
    uint32_t read_uint32_le(QFile& file);

    // Read binary data from address
    QByteArray read_data_at_address(QFile& file, uint32_t address);

    // Decode RLE-compressed bitmap data
    QByteArray decode_rle(const QByteArray& compressed_data, int expected_size, bool is_x2_device = false);

    // Convert decoded bitmap to QImage with transparency
    QImage bitmap_to_image(const QByteArray& bitmap_data, int width, int height);

    // Map color code to RGB value
    uint32_t color_code_to_rgb(uint8_t color_code, bool is_x2_device = false);

    QString file_path_;
    MarkFileData file_data_;
    std::set<int> page_set_;
    std::map<int, QPixmap> cached_pixmaps_;
};
