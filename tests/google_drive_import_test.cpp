#include <cstdlib>
#include <iostream>

#include <QCoreApplication>
#include <QUrlQuery>

#include "google_drive_import.h"

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "google_drive_import_test failed: " << message << "\n";
        std::exit(1);
    }
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    std::optional<GoogleDriveSource> file_source = parse_google_drive_source(
        L"https://drive.google.com/file/d/1AbC_defGhijKlmnopQRstuVWxyz/view?usp=sharing&resourcekey=0-driveKey",
        GoogleDriveSourceKind::File);
    expect(file_source.has_value(), "file URL parses");
    expect(file_source->id == "1AbC_defGhijKlmnopQRstuVWxyz", "file id is extracted from /file/d URL");
    expect(file_source->resource_key == "0-driveKey", "resource key is extracted");

    std::optional<GoogleDriveSource> open_source = parse_google_drive_source(
        L"https://drive.google.com/open?id=1OpenId_abcdefghijklmno",
        GoogleDriveSourceKind::File);
    expect(open_source.has_value(), "open?id URL parses");
    expect(open_source->id == "1OpenId_abcdefghijklmno", "file id is extracted from id query");

    std::optional<GoogleDriveSource> folder_source = parse_google_drive_source(
        L"https://drive.google.com/drive/u/0/folders/1Folder_abcdefghijklmno?resourcekey=0-folderKey",
        GoogleDriveSourceKind::Folder);
    expect(folder_source.has_value(), "folder URL parses");
    expect(folder_source->id == "1Folder_abcdefghijklmno", "folder id is extracted");
    expect(folder_source->resource_key == "0-folderKey", "folder resource key is extracted");

    std::optional<GoogleDriveSource> raw_source = parse_google_drive_source(L"1RawDriveId_abcdefghi", GoogleDriveSourceKind::File);
    expect(raw_source.has_value(), "raw ID parses");
    expect(raw_source->id == "1RawDriveId_abcdefghi", "raw ID is retained");

    expect(!parse_google_drive_source(L"not a drive source", GoogleDriveSourceKind::File).has_value(), "invalid source is rejected");

    QUrl file_url = google_drive_file_download_url(file_source.value(), "api-key");
    expect(file_url.toString().startsWith("https://www.googleapis.com/drive/v3/files/1AbC_defGhijKlmnopQRstuVWxyz"), "API download URL uses files.get endpoint");
    QUrlQuery file_query(file_url);
    expect(file_query.queryItemValue("alt") == "media", "API download URL requests media");
    expect(file_query.queryItemValue("key") == "api-key", "API download URL includes API key");

    QUrl public_file_url = google_drive_file_download_url(file_source.value(), "");
    expect(public_file_url.host() == "drive.google.com", "public download URL uses Drive web endpoint");
    QUrlQuery public_file_query(public_file_url);
    expect(public_file_query.queryItemValue("id") == file_source->id, "public download URL includes file id");
    expect(public_file_query.queryItemValue("resourcekey") == file_source->resource_key, "public download URL includes resource key");

    QUrl folder_url = google_drive_folder_list_url(folder_source.value(), "api-key", "next-page");
    expect(folder_url.host() == "www.googleapis.com", "folder list URL uses Drive API host");
    QUrlQuery folder_query(folder_url);
    expect(folder_query.queryItemValue("key") == "api-key", "folder list URL includes API key");
    expect(folder_query.queryItemValue("pageToken") == "next-page", "folder list URL includes page token");
    expect(folder_query.queryItemValue("q").contains("'1Folder_abcdefghijklmno' in parents"), "folder list URL filters by parent");
    expect(folder_query.queryItemValue("q").contains("application/pdf"), "folder list URL filters PDFs");

    expect(google_drive_resource_key_header(file_source.value()) == "1AbC_defGhijKlmnopQRstuVWxyz/0-driveKey", "resource key header is built");
    expect(google_drive_file_name_for_import("Paper: Name.PDF", "1AbCdefGhijklmno") == "Paper_ Name [gdrive-1AbCdefGhijk].pdf", "import file name is sanitized and tagged");
    expect(google_drive_file_name_from_content_disposition("attachment; filename=\"Downloaded Paper.pdf\"") == "Downloaded Paper.pdf", "quoted filename is parsed");
    expect(google_drive_file_name_from_content_disposition("attachment; filename*=UTF-8''Downloaded%20Paper.pdf") == "Downloaded Paper.pdf", "UTF-8 filename is parsed");

    std::cout << "google_drive_import_test passed\n";
    return 0;
}
