#pragma once

#include <optional>
#include <string>

#include <QByteArray>
#include <QString>
#include <QUrl>

enum class GoogleDriveSourceKind {
    File,
    Folder,
};

struct GoogleDriveSource {
    GoogleDriveSourceKind kind = GoogleDriveSourceKind::File;
    QString id;
    QString resource_key;
    QString original_input;

    bool is_valid() const;
};

std::optional<GoogleDriveSource> parse_google_drive_source(const std::wstring& input, GoogleDriveSourceKind expected_kind);
QUrl google_drive_file_download_url(const GoogleDriveSource& source, const QString& api_key);
QUrl google_drive_folder_list_url(const GoogleDriveSource& source, const QString& api_key, const QString& page_token = QString());
QByteArray google_drive_resource_key_header(const GoogleDriveSource& source);
QString google_drive_file_name_for_import(const QString& preferred_name, const QString& file_id);
QString google_drive_file_name_from_content_disposition(const QString& content_disposition);
