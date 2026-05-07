#pragma once

#include <optional>
#include <string>
#include <vector>

struct LibraryEntry {
    std::wstring path;
    std::wstring display_name;
    std::string date_added;
    std::optional<std::string> last_opened;
    std::vector<std::wstring> tags;
    std::vector<std::wstring> collections;
};

enum class LibraryAddResult {
    Added,
    AlreadyExists,
    InvalidPath,
    SaveFailed,
};

enum class LibraryRemoveResult {
    Removed,
    NotFound,
    InvalidPath,
    SaveFailed,
};

enum class LibraryCreateCollectionResult {
    Created,
    AlreadyExists,
    InvalidName,
    SaveFailed,
};

enum class LibraryAddToCollectionResult {
    Added,
    AlreadyInCollection,
    DocumentNotFound,
    InvalidCollectionName,
    SaveFailed,
};

enum class LibraryRemoveFromCollectionResult {
    Removed,
    DocumentNotFound,
    CollectionNotFound,
    NotInCollection,
    InvalidCollectionName,
    SaveFailed,
};

enum class LibraryImportFileResult {
    Added,
    AlreadyExists,
    InvalidPath,
    FileNotFound,
    NotAFile,
    NotPdf,
    SaveFailed,
};

enum class LibraryImportSourceStatus {
    Ok,
    InvalidPath,
    NotFound,
    NotDirectory,
    InvalidCollectionName,
    SaveFailed,
};

struct LibraryImportSummary {
    LibraryImportSourceStatus source_status = LibraryImportSourceStatus::Ok;
    int files_found = 0;
    int files_added = 0;
    int files_already_present = 0;
    int files_added_to_collection = 0;
    int files_already_in_collection = 0;
    int files_skipped_invalid = 0;
    int files_skipped_not_pdf = 0;
    int files_skipped_error = 0;
    int collections_created = 0;
    bool collection_created = false;
};

class LibraryManager {
private:
    std::wstring library_file_path;
    std::vector<LibraryEntry> library_entries;
    std::vector<std::wstring> library_collections;
    std::wstring last_error_message;

    std::vector<LibraryEntry>::iterator find_entry(const std::wstring& canonical_path);
    std::vector<LibraryEntry>::const_iterator find_entry(const std::wstring& canonical_path) const;
    std::vector<std::wstring>::iterator find_collection(const std::wstring& collection_name);
    std::vector<std::wstring>::const_iterator find_collection(const std::wstring& collection_name) const;
    void set_last_error(const std::wstring& message);

public:
    explicit LibraryManager(const std::wstring& library_file_path);

    bool load();
    bool save();

    LibraryAddResult add_document(const std::wstring& path);
    LibraryRemoveResult remove_document(const std::wstring& path);
    bool update_last_opened(const std::wstring& path);

    LibraryImportFileResult import_file(const std::wstring& path);
    LibraryImportSummary import_folder(const std::wstring& folder_path, bool recursive = false);
    LibraryImportSummary import_folder_to_collection(const std::wstring& folder_path, const std::wstring& collection_name, bool recursive = false);

    LibraryCreateCollectionResult create_collection(const std::wstring& collection_name);
    LibraryAddToCollectionResult add_document_to_collection(const std::wstring& path, const std::wstring& collection_name);
    LibraryRemoveFromCollectionResult remove_document_from_collection(const std::wstring& path, const std::wstring& collection_name);

    bool contains(const std::wstring& path) const;
    bool is_document_in_library(const std::wstring& path) const;
    std::optional<LibraryEntry> get_document_info(const std::wstring& path) const;
    bool collection_exists(const std::wstring& collection_name) const;
    const std::vector<LibraryEntry>& entries() const;
    const std::vector<std::wstring>& list_collections() const;
    std::vector<LibraryEntry> documents_in_collection(const std::wstring& collection_name) const;
    std::vector<LibraryEntry> uncategorized_documents() const;
    const std::wstring& last_error() const;
    const std::wstring& storage_path() const;

    static std::wstring canonicalize_path(const std::wstring& path);
    static std::wstring normalize_library_path(const std::wstring& path);
    static std::wstring normalize_collection_name(const std::wstring& collection_name);
    static bool is_pdf_file(const std::wstring& path);
};
