#include "google_drive_import.h"

#include <vector>

#include <QFileInfo>
#include <QRegularExpression>
#include <QUrlQuery>

namespace {

QString trimmed_input(const std::wstring& input) {
    return QString::fromStdWString(input).trimmed();
}

bool looks_like_drive_id(const QString& value) {
    static const QRegularExpression id_expression("^[A-Za-z0-9_-]{10,}$");
    return id_expression.match(value).hasMatch();
}

QString query_item_case_insensitive(const QUrlQuery& query, const QString& key) {
    const auto items = query.queryItems();
    for (const auto& item : items) {
        if (item.first.compare(key, Qt::CaseInsensitive) == 0) {
            return item.second;
        }
    }
    return QString();
}

QString match_first_capture(const QString& text, const std::vector<QRegularExpression>& patterns) {
    for (const QRegularExpression& pattern : patterns) {
        QRegularExpressionMatch match = pattern.match(text);
        if (match.hasMatch()) {
            return match.captured(1);
        }
    }
    return QString();
}

QString extract_file_id_from_path(const QString& path) {
    return match_first_capture(path, {
        QRegularExpression("/file/d/([^/?#]+)"),
        QRegularExpression("/uc/([^/?#]+)"),
    });
}

QString extract_folder_id_from_path(const QString& path) {
    return match_first_capture(path, {
        QRegularExpression("/drive/(?:u/\\d+/)?folders/([^/?#]+)"),
        QRegularExpression("/folders/([^/?#]+)"),
    });
}

QString sanitize_file_name(QString file_name) {
    file_name = file_name.trimmed();
    file_name.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
    file_name.replace(QRegularExpression("\\s+"), " ");
    while (file_name.startsWith(".")) {
        file_name.remove(0, 1);
    }
    if (file_name.isEmpty()) {
        file_name = "google-drive.pdf";
    }
    return file_name;
}

QString short_drive_id(const QString& file_id) {
    if (file_id.size() <= 12) {
        return file_id;
    }
    return file_id.left(12);
}

} // namespace

bool GoogleDriveSource::is_valid() const {
    return looks_like_drive_id(id);
}

std::optional<GoogleDriveSource> parse_google_drive_source(const std::wstring& input, GoogleDriveSourceKind expected_kind) {
    QString raw_input = trimmed_input(input);
    if (raw_input.isEmpty()) {
        return {};
    }

    GoogleDriveSource source;
    source.kind = expected_kind;
    source.original_input = raw_input;

    QUrl url(raw_input);
    if (url.isValid() && !url.scheme().isEmpty() && !url.host().isEmpty()) {
        QUrlQuery query(url);
        source.resource_key = query_item_case_insensitive(query, "resourcekey");

        QString id_from_query = query_item_case_insensitive(query, "id");
        if (looks_like_drive_id(id_from_query)) {
            source.id = id_from_query;
        }
        else if (expected_kind == GoogleDriveSourceKind::Folder) {
            source.id = extract_folder_id_from_path(url.path());
        }
        else {
            source.id = extract_file_id_from_path(url.path());
        }

        if (source.is_valid()) {
            return source;
        }
        return {};
    }

    if (looks_like_drive_id(raw_input)) {
        source.id = raw_input;
        return source;
    }

    return {};
}

QUrl google_drive_file_download_url(const GoogleDriveSource& source, const QString& api_key) {
    QString trimmed_api_key = api_key.trimmed();
    if (!trimmed_api_key.isEmpty()) {
        QUrl url(QString("https://www.googleapis.com/drive/v3/files/") + source.id);
        QUrlQuery query;
        query.addQueryItem("alt", "media");
        query.addQueryItem("supportsAllDrives", "true");
        query.addQueryItem("key", trimmed_api_key);
        url.setQuery(query);
        return url;
    }

    QUrl url("https://drive.google.com/uc");
    QUrlQuery query;
    query.addQueryItem("export", "download");
    query.addQueryItem("id", source.id);
    if (!source.resource_key.isEmpty()) {
        query.addQueryItem("resourcekey", source.resource_key);
    }
    url.setQuery(query);
    return url;
}

QUrl google_drive_folder_list_url(const GoogleDriveSource& source, const QString& api_key, const QString& page_token) {
    QUrl url("https://www.googleapis.com/drive/v3/files");
    QUrlQuery query;
    query.addQueryItem("key", api_key.trimmed());
    query.addQueryItem("supportsAllDrives", "true");
    query.addQueryItem("includeItemsFromAllDrives", "true");
    query.addQueryItem("pageSize", "1000");
    query.addQueryItem("fields", "nextPageToken,files(id,name,mimeType,resourceKey)");
    query.addQueryItem("q", QString("'%1' in parents and trashed = false and mimeType = 'application/pdf'").arg(source.id));
    if (!page_token.isEmpty()) {
        query.addQueryItem("pageToken", page_token);
    }
    url.setQuery(query);
    return url;
}

QByteArray google_drive_resource_key_header(const GoogleDriveSource& source) {
    if (source.id.isEmpty() || source.resource_key.isEmpty()) {
        return QByteArray();
    }
    return QString("%1/%2").arg(source.id, source.resource_key).toUtf8();
}

QString google_drive_file_name_for_import(const QString& preferred_name, const QString& file_id) {
    QString sanitized_preferred_name = sanitize_file_name(preferred_name);
    QFileInfo file_info(sanitized_preferred_name);
    QString base_name = file_info.completeBaseName().trimmed();
    if (base_name.isEmpty() || sanitized_preferred_name == "google-drive.pdf") {
        base_name = "google-drive";
    }

    QString suffix = file_info.suffix();
    if (suffix.compare("pdf", Qt::CaseInsensitive) != 0) {
        suffix = "pdf";
    }

    QString drive_suffix = short_drive_id(file_id);
    QString drive_tag = QString("[gdrive-") + drive_suffix + "]";
    if (!drive_suffix.isEmpty() && !base_name.contains(drive_tag)) {
        base_name += QString(" ") + drive_tag;
    }
    return sanitize_file_name(base_name + "." + suffix.toLower());
}

QString google_drive_file_name_from_content_disposition(const QString& content_disposition) {
    if (content_disposition.isEmpty()) {
        return QString();
    }

    QRegularExpression utf8_filename_expression("filename\\*=UTF-8''([^;]+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch utf8_match = utf8_filename_expression.match(content_disposition);
    if (utf8_match.hasMatch()) {
        return QUrl::fromPercentEncoding(utf8_match.captured(1).toUtf8()).trimmed();
    }

    QRegularExpression quoted_filename_expression("filename=\"([^\"]+)\"", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch quoted_match = quoted_filename_expression.match(content_disposition);
    if (quoted_match.hasMatch()) {
        return quoted_match.captured(1).trimmed();
    }

    QRegularExpression filename_expression("filename=([^;]+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = filename_expression.match(content_disposition);
    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }

    return QString();
}
