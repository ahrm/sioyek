#include "library_manager.h"

#include <algorithm>
#include <set>

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSaveFile>
#include <QString>
#include <QStringList>

namespace {

std::string current_timestamp() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs).toStdString();
}

std::wstring default_display_name(const std::wstring& path) {
    QString file_name = QFileInfo(QString::fromStdWString(path)).fileName();
    if (file_name.isEmpty()) {
        return path;
    }
    return file_name.toStdWString();
}

QJsonArray wstring_vector_to_json(const std::vector<std::wstring>& values) {
    QJsonArray array;
    for (const std::wstring& value : values) {
        array.append(QString::fromStdWString(value));
    }
    return array;
}

std::vector<std::wstring> json_to_wstring_vector(const QJsonValue& value) {
    std::vector<std::wstring> result;
    if (!value.isArray()) {
        return result;
    }

    QJsonArray array = value.toArray();
    for (const QJsonValue& item : array) {
        if (item.isString()) {
            result.push_back(item.toString().toStdWString());
        }
    }
    return result;
}

bool vector_contains(const std::vector<std::wstring>& values, const std::wstring& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

void count_import_file_result(LibraryImportSummary& summary, LibraryImportFileResult result) {
    if (result == LibraryImportFileResult::Added) {
        summary.files_added++;
    }
    else if (result == LibraryImportFileResult::AlreadyExists) {
        summary.files_already_present++;
    }
    else if (result == LibraryImportFileResult::InvalidPath) {
        summary.files_skipped_invalid++;
    }
    else if (result == LibraryImportFileResult::NotPdf) {
        summary.files_skipped_not_pdf++;
    }
    else {
        summary.files_skipped_error++;
    }
}

void append_pdf_path_if_importable(const QFileInfo& file_info, std::vector<std::wstring>& pdf_paths, LibraryImportSummary& summary) {
    if (!file_info.isFile()) {
        summary.files_skipped_invalid++;
        return;
    }

    if (file_info.fileName().startsWith(".")) {
        return;
    }

    std::wstring file_path = file_info.absoluteFilePath().toStdWString();
    if (!LibraryManager::is_pdf_file(file_path)) {
        summary.files_skipped_not_pdf++;
        return;
    }

    summary.files_found++;
    pdf_paths.push_back(file_path);
}

std::wstring relative_subfolder_collection_name(const std::wstring& root_folder_path, const std::wstring& file_path) {
    QFileInfo root_info(QString::fromStdWString(root_folder_path));
    QFileInfo file_info(QString::fromStdWString(file_path));
    QString root_path = root_info.absoluteFilePath();
    QString parent_path = file_info.absoluteDir().absolutePath();
    if (root_path.isEmpty() || parent_path.isEmpty() || QDir::cleanPath(root_path) == QDir::cleanPath(parent_path)) {
        return L"";
    }

    QString relative_path = QDir(root_path).relativeFilePath(parent_path);
    relative_path = QDir::cleanPath(relative_path);
    if (relative_path.isEmpty() || relative_path == "." || relative_path == ".." || relative_path.startsWith("../") || relative_path.startsWith("..\\")) {
        return L"";
    }

    relative_path.replace('\\', '/');
    QStringList parts = relative_path.split('/', Qt::SkipEmptyParts);
    return LibraryManager::normalize_collection_name(parts.join("-").toStdWString());
}

std::wstring collection_name_for_imported_file(
    const std::wstring& root_collection_name,
    const std::wstring& relative_collection_name
) {
    if (root_collection_name.empty()) {
        return relative_collection_name;
    }
    if (relative_collection_name.empty()) {
        return root_collection_name;
    }
    return root_collection_name + L"-" + relative_collection_name;
}

void add_imported_pdf_to_collection(
    LibraryManager* manager,
    const std::wstring& pdf_path,
    const std::wstring& collection_name,
    LibraryImportSummary& summary
) {
    if (!manager || collection_name.empty()) {
        return;
    }

    bool collection_already_exists = manager->collection_exists(collection_name);
    LibraryAddToCollectionResult collection_result = manager->add_document_to_collection(pdf_path, collection_name);
    if (collection_result == LibraryAddToCollectionResult::Added) {
        summary.files_added_to_collection++;
        if (!collection_already_exists) {
            summary.collections_created++;
            summary.collection_created = true;
        }
    }
    else if (collection_result == LibraryAddToCollectionResult::AlreadyInCollection) {
        summary.files_already_in_collection++;
    }
    else {
        summary.files_skipped_error++;
        if (collection_result == LibraryAddToCollectionResult::SaveFailed) {
            summary.source_status = LibraryImportSourceStatus::SaveFailed;
        }
    }
}

LibraryImportSourceStatus collect_pdf_paths(const std::wstring& folder_path, bool recursive, std::vector<std::wstring>& pdf_paths, LibraryImportSummary& summary) {
    std::wstring normalized_folder = LibraryManager::normalize_library_path(folder_path);
    if (normalized_folder.empty()) {
        return LibraryImportSourceStatus::InvalidPath;
    }

    QFileInfo folder_info(QString::fromStdWString(normalized_folder));
    if (!folder_info.exists()) {
        return LibraryImportSourceStatus::NotFound;
    }
    if (!folder_info.isDir()) {
        return LibraryImportSourceStatus::NotDirectory;
    }

    if (recursive) {
        QDirIterator iterator(
            folder_info.absoluteFilePath(),
            QDir::Files | QDir::NoDotAndDotDot | QDir::Readable,
            QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            iterator.next();
            append_pdf_path_if_importable(iterator.fileInfo(), pdf_paths, summary);
        }
    }
    else {
        QDir dir(folder_info.absoluteFilePath());
        QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
        for (const QFileInfo& file_info : files) {
            append_pdf_path_if_importable(file_info, pdf_paths, summary);
        }
    }

    return LibraryImportSourceStatus::Ok;
}

QJsonObject entry_to_json(const LibraryEntry& entry) {
    QJsonObject object;
    object["path"] = QString::fromStdWString(entry.path);
    object["display_name"] = QString::fromStdWString(entry.display_name);
    object["date_added"] = QString::fromStdString(entry.date_added);
    object["tags"] = wstring_vector_to_json(entry.tags);
    object["collections"] = wstring_vector_to_json(entry.collections);
    if (entry.last_opened.has_value()) {
        object["last_opened"] = QString::fromStdString(entry.last_opened.value());
    }
    else {
        object["last_opened"] = QJsonValue::Null;
    }
    return object;
}

std::optional<LibraryEntry> entry_from_json(const QJsonValue& value) {
    if (!value.isObject()) {
        return {};
    }

    QJsonObject object = value.toObject();
    if (!object.value("path").isString()) {
        return {};
    }

    std::wstring canonical_path = LibraryManager::canonicalize_path(object.value("path").toString().toStdWString());
    if (canonical_path.empty()) {
        return {};
    }

    LibraryEntry entry;
    entry.path = canonical_path;
    entry.display_name = object.value("display_name").isString()
        ? object.value("display_name").toString().toStdWString()
        : default_display_name(canonical_path);
    if (entry.display_name.empty()) {
        entry.display_name = default_display_name(canonical_path);
    }
    entry.date_added = object.value("date_added").isString()
        ? object.value("date_added").toString().toStdString()
        : current_timestamp();
    if (object.value("last_opened").isString()) {
        entry.last_opened = object.value("last_opened").toString().toStdString();
    }
    entry.tags = json_to_wstring_vector(object.value("tags"));
    entry.collections = json_to_wstring_vector(object.value("collections"));
    return entry;
}

}

LibraryManager::LibraryManager(const std::wstring& library_file_path_) : library_file_path(library_file_path_) {
}

std::vector<LibraryEntry>::iterator LibraryManager::find_entry(const std::wstring& canonical_path) {
    return std::find_if(library_entries.begin(), library_entries.end(), [&](const LibraryEntry& entry) {
        return entry.path == canonical_path;
        });
}

std::vector<LibraryEntry>::const_iterator LibraryManager::find_entry(const std::wstring& canonical_path) const {
    return std::find_if(library_entries.begin(), library_entries.end(), [&](const LibraryEntry& entry) {
        return entry.path == canonical_path;
        });
}

void LibraryManager::set_last_error(const std::wstring& message) {
    last_error_message = message;
}

bool LibraryManager::load() {
    library_entries.clear();
    library_collections.clear();
    last_error_message.clear();

    QFile file(QString::fromStdWString(library_file_path));
    if (!file.exists()) {
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        set_last_error(L"Could not read library file: " + library_file_path);
        return false;
    }

    QJsonParseError parse_error;
    QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
        set_last_error(L"Could not parse library file: " + parse_error.errorString().toStdWString());
        library_entries.clear();
        return false;
    }

    QJsonValue entries_value = document.object().value("entries");
    if (!entries_value.isArray()) {
        set_last_error(L"Library file does not contain an entries array");
        library_entries.clear();
        return false;
    }

    if (document.object().value("collections").isArray()) {
        for (const std::wstring& collection_name : json_to_wstring_vector(document.object().value("collections"))) {
            std::wstring normalized_name = normalize_collection_name(collection_name);
            if (!normalized_name.empty() && !vector_contains(library_collections, normalized_name)) {
                library_collections.push_back(normalized_name);
            }
        }
    }

    std::set<std::wstring> seen_paths;
    for (const QJsonValue& value : entries_value.toArray()) {
        std::optional<LibraryEntry> entry = entry_from_json(value);
        if (entry.has_value() && seen_paths.find(entry->path) == seen_paths.end()) {
            std::vector<std::wstring> normalized_collections;
            for (const std::wstring& collection_name : entry->collections) {
                std::wstring normalized_name = normalize_collection_name(collection_name);
                if (!normalized_name.empty() && !vector_contains(normalized_collections, normalized_name)) {
                    normalized_collections.push_back(normalized_name);
                    if (!vector_contains(library_collections, normalized_name)) {
                        library_collections.push_back(normalized_name);
                    }
                }
            }
            entry->collections = normalized_collections;
            seen_paths.insert(entry->path);
            library_entries.push_back(entry.value());
        }
    }
    return true;
}

bool LibraryManager::save() {
    last_error_message.clear();

    QJsonObject root;
    root["version"] = 1;
    root["collections"] = wstring_vector_to_json(library_collections);

    QJsonArray entries_array;
    for (const LibraryEntry& entry : library_entries) {
        entries_array.append(entry_to_json(entry));
    }
    root["entries"] = entries_array;

    QFileInfo file_info(QString::fromStdWString(library_file_path));
    if (!file_info.dir().exists() && !file_info.dir().mkpath(".")) {
        set_last_error(L"Could not create library directory: " + file_info.dir().absolutePath().toStdWString());
        return false;
    }

    QSaveFile file(QString::fromStdWString(library_file_path));
    if (!file.open(QIODevice::WriteOnly)) {
        set_last_error(L"Could not write library file: " + library_file_path);
        return false;
    }

    QJsonDocument document(root);
    file.write(document.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        set_last_error(L"Could not commit library file: " + library_file_path);
        return false;
    }
    return true;
}

LibraryAddResult LibraryManager::add_document(const std::wstring& path) {
    std::wstring canonical_path = canonicalize_path(path);
    if (canonical_path.empty()) {
        return LibraryAddResult::InvalidPath;
    }
    if (find_entry(canonical_path) != library_entries.end()) {
        return LibraryAddResult::AlreadyExists;
    }

    LibraryEntry entry;
    entry.path = canonical_path;
    entry.display_name = default_display_name(canonical_path);
    entry.date_added = current_timestamp();
    entry.tags = {};
    entry.collections = {};

    library_entries.push_back(entry);
    if (!save()) {
        library_entries.pop_back();
        return LibraryAddResult::SaveFailed;
    }
    return LibraryAddResult::Added;
}

LibraryRemoveResult LibraryManager::remove_document(const std::wstring& path) {
    std::wstring canonical_path = canonicalize_path(path);
    if (canonical_path.empty()) {
        return LibraryRemoveResult::InvalidPath;
    }

    auto it = find_entry(canonical_path);
    if (it == library_entries.end()) {
        return LibraryRemoveResult::NotFound;
    }

    LibraryEntry removed_entry = *it;
    int removed_index = static_cast<int>(it - library_entries.begin());
    library_entries.erase(it);
    if (!save()) {
        library_entries.insert(library_entries.begin() + removed_index, removed_entry);
        return LibraryRemoveResult::SaveFailed;
    }
    return LibraryRemoveResult::Removed;
}

bool LibraryManager::update_last_opened(const std::wstring& path) {
    std::wstring canonical_path = canonicalize_path(path);
    if (canonical_path.empty()) {
        return false;
    }

    auto it = find_entry(canonical_path);
    if (it == library_entries.end()) {
        return false;
    }

    std::optional<std::string> previous_last_opened = it->last_opened;
    it->last_opened = current_timestamp();
    if (!save()) {
        it->last_opened = previous_last_opened;
        return false;
    }
    return true;
}

LibraryImportFileResult LibraryManager::import_file(const std::wstring& path) {
    std::wstring canonical_path = normalize_library_path(path);
    if (canonical_path.empty()) {
        return LibraryImportFileResult::InvalidPath;
    }

    QFileInfo file_info(QString::fromStdWString(canonical_path));
    if (!file_info.exists()) {
        return LibraryImportFileResult::FileNotFound;
    }
    if (!file_info.isFile()) {
        return LibraryImportFileResult::NotAFile;
    }
    if (!is_pdf_file(canonical_path)) {
        return LibraryImportFileResult::NotPdf;
    }

    LibraryAddResult add_result = add_document(canonical_path);
    if (add_result == LibraryAddResult::Added) {
        return LibraryImportFileResult::Added;
    }
    if (add_result == LibraryAddResult::AlreadyExists) {
        return LibraryImportFileResult::AlreadyExists;
    }
    if (add_result == LibraryAddResult::InvalidPath) {
        return LibraryImportFileResult::InvalidPath;
    }
    return LibraryImportFileResult::SaveFailed;
}

LibraryImportSummary LibraryManager::import_folder(const std::wstring& folder_path, bool recursive) {
    LibraryImportSummary summary;
    std::vector<std::wstring> pdf_paths;
    std::wstring normalized_folder = normalize_library_path(folder_path);
    summary.source_status = collect_pdf_paths(normalized_folder, recursive, pdf_paths, summary);
    if (summary.source_status != LibraryImportSourceStatus::Ok) {
        return summary;
    }

    for (const std::wstring& pdf_path : pdf_paths) {
        LibraryImportFileResult import_result = import_file(pdf_path);
        count_import_file_result(summary, import_result);
        if (recursive && (import_result == LibraryImportFileResult::Added || import_result == LibraryImportFileResult::AlreadyExists)) {
            std::wstring relative_collection = relative_subfolder_collection_name(normalized_folder, pdf_path);
            add_imported_pdf_to_collection(this, pdf_path, relative_collection, summary);
        }
    }
    return summary;
}

LibraryImportSummary LibraryManager::import_folder_to_collection(const std::wstring& folder_path, const std::wstring& collection_name, bool recursive) {
    LibraryImportSummary summary;
    std::wstring normalized_name = normalize_collection_name(collection_name);
    if (normalized_name.empty()) {
        summary.source_status = LibraryImportSourceStatus::InvalidCollectionName;
        return summary;
    }

    std::vector<std::wstring> pdf_paths;
    std::wstring normalized_folder = normalize_library_path(folder_path);
    summary.source_status = collect_pdf_paths(normalized_folder, recursive, pdf_paths, summary);
    if (summary.source_status != LibraryImportSourceStatus::Ok) {
        return summary;
    }

    bool had_collection = collection_exists(normalized_name);
    LibraryCreateCollectionResult create_result = create_collection(normalized_name);
    if (create_result == LibraryCreateCollectionResult::Created) {
        summary.collection_created = true;
        summary.collections_created++;
    }
    else if (create_result == LibraryCreateCollectionResult::SaveFailed) {
        summary.source_status = LibraryImportSourceStatus::SaveFailed;
        return summary;
    }
    else if (create_result == LibraryCreateCollectionResult::InvalidName) {
        summary.source_status = LibraryImportSourceStatus::InvalidCollectionName;
        return summary;
    }
    else if (had_collection && create_result == LibraryCreateCollectionResult::AlreadyExists) {
        summary.collection_created = false;
    }

    for (const std::wstring& pdf_path : pdf_paths) {
        LibraryImportFileResult import_result = import_file(pdf_path);
        count_import_file_result(summary, import_result);
        if (import_result != LibraryImportFileResult::Added && import_result != LibraryImportFileResult::AlreadyExists) {
            continue;
        }

        std::wstring relative_collection = recursive ? relative_subfolder_collection_name(normalized_folder, pdf_path) : L"";
        std::wstring target_collection = collection_name_for_imported_file(normalized_name, relative_collection);
        add_imported_pdf_to_collection(this, pdf_path, target_collection, summary);
    }

    return summary;
}

std::vector<std::wstring>::iterator LibraryManager::find_collection(const std::wstring& collection_name) {
    return std::find(library_collections.begin(), library_collections.end(), collection_name);
}

std::vector<std::wstring>::const_iterator LibraryManager::find_collection(const std::wstring& collection_name) const {
    return std::find(library_collections.begin(), library_collections.end(), collection_name);
}

LibraryCreateCollectionResult LibraryManager::create_collection(const std::wstring& collection_name) {
    std::wstring normalized_name = normalize_collection_name(collection_name);
    if (normalized_name.empty()) {
        return LibraryCreateCollectionResult::InvalidName;
    }
    if (find_collection(normalized_name) != library_collections.end()) {
        return LibraryCreateCollectionResult::AlreadyExists;
    }

    library_collections.push_back(normalized_name);
    if (!save()) {
        library_collections.pop_back();
        return LibraryCreateCollectionResult::SaveFailed;
    }
    return LibraryCreateCollectionResult::Created;
}

LibraryAddToCollectionResult LibraryManager::add_document_to_collection(const std::wstring& path, const std::wstring& collection_name) {
    std::wstring canonical_path = canonicalize_path(path);
    std::wstring normalized_name = normalize_collection_name(collection_name);
    if (canonical_path.empty() || normalized_name.empty()) {
        return LibraryAddToCollectionResult::InvalidCollectionName;
    }

    auto entry_it = find_entry(canonical_path);
    if (entry_it == library_entries.end()) {
        return LibraryAddToCollectionResult::DocumentNotFound;
    }
    if (vector_contains(entry_it->collections, normalized_name)) {
        return LibraryAddToCollectionResult::AlreadyInCollection;
    }

    bool collection_was_created = false;
    if (find_collection(normalized_name) == library_collections.end()) {
        library_collections.push_back(normalized_name);
        collection_was_created = true;
    }

    entry_it->collections.push_back(normalized_name);
    if (!save()) {
        entry_it->collections.pop_back();
        if (collection_was_created) {
            library_collections.pop_back();
        }
        return LibraryAddToCollectionResult::SaveFailed;
    }
    return LibraryAddToCollectionResult::Added;
}

LibraryRemoveFromCollectionResult LibraryManager::remove_document_from_collection(const std::wstring& path, const std::wstring& collection_name) {
    std::wstring canonical_path = canonicalize_path(path);
    std::wstring normalized_name = normalize_collection_name(collection_name);
    if (canonical_path.empty() || normalized_name.empty()) {
        return LibraryRemoveFromCollectionResult::InvalidCollectionName;
    }

    auto entry_it = find_entry(canonical_path);
    if (entry_it == library_entries.end()) {
        return LibraryRemoveFromCollectionResult::DocumentNotFound;
    }
    if (find_collection(normalized_name) == library_collections.end()) {
        return LibraryRemoveFromCollectionResult::CollectionNotFound;
    }

    auto collection_it = std::find(entry_it->collections.begin(), entry_it->collections.end(), normalized_name);
    if (collection_it == entry_it->collections.end()) {
        return LibraryRemoveFromCollectionResult::NotInCollection;
    }

    int removed_index = static_cast<int>(collection_it - entry_it->collections.begin());
    entry_it->collections.erase(collection_it);
    if (!save()) {
        entry_it->collections.insert(entry_it->collections.begin() + removed_index, normalized_name);
        return LibraryRemoveFromCollectionResult::SaveFailed;
    }
    return LibraryRemoveFromCollectionResult::Removed;
}

bool LibraryManager::contains(const std::wstring& path) const {
    std::wstring canonical_path = canonicalize_path(path);
    if (canonical_path.empty()) {
        return false;
    }
    return find_entry(canonical_path) != library_entries.end();
}

bool LibraryManager::is_document_in_library(const std::wstring& path) const {
    return contains(path);
}

std::optional<LibraryEntry> LibraryManager::get_document_info(const std::wstring& path) const {
    std::wstring canonical_path = canonicalize_path(path);
    if (canonical_path.empty()) {
        return {};
    }

    auto entry_it = find_entry(canonical_path);
    if (entry_it == library_entries.end()) {
        return {};
    }
    return *entry_it;
}

bool LibraryManager::collection_exists(const std::wstring& collection_name) const {
    std::wstring normalized_name = normalize_collection_name(collection_name);
    if (normalized_name.empty()) {
        return false;
    }
    return find_collection(normalized_name) != library_collections.end();
}

const std::vector<LibraryEntry>& LibraryManager::entries() const {
    return library_entries;
}

const std::vector<std::wstring>& LibraryManager::list_collections() const {
    return library_collections;
}

std::vector<LibraryEntry> LibraryManager::documents_in_collection(const std::wstring& collection_name) const {
    std::vector<LibraryEntry> result;
    std::wstring normalized_name = normalize_collection_name(collection_name);
    if (normalized_name.empty()) {
        return result;
    }

    for (const LibraryEntry& entry : library_entries) {
        if (vector_contains(entry.collections, normalized_name)) {
            result.push_back(entry);
        }
    }
    return result;
}

std::vector<LibraryEntry> LibraryManager::uncategorized_documents() const {
    std::vector<LibraryEntry> result;
    for (const LibraryEntry& entry : library_entries) {
        if (entry.collections.empty()) {
            result.push_back(entry);
        }
    }
    return result;
}

const std::wstring& LibraryManager::last_error() const {
    return last_error_message;
}

const std::wstring& LibraryManager::storage_path() const {
    return library_file_path;
}

std::wstring LibraryManager::canonicalize_path(const std::wstring& path) {
    return normalize_library_path(path);
}

std::wstring LibraryManager::normalize_library_path(const std::wstring& path) {
    QString normalized_input = QString::fromStdWString(path).trimmed();
    if (normalized_input.isEmpty()) {
        return L"";
    }
#ifdef SIOYEK_ANDROID
    if (normalized_input.startsWith(":") || normalized_input.startsWith("content:/")) {
        return normalized_input.toStdWString();
    }
#endif

    if (normalized_input.startsWith("~")) {
        normalized_input = QDir::homePath() + normalized_input.mid(1);
    }

    QFileInfo file_info(normalized_input);
    QString normalized_path = file_info.exists() ? file_info.canonicalFilePath() : file_info.absoluteFilePath();
    if (normalized_path.isEmpty()) {
        normalized_path = QDir(normalized_input).absolutePath();
    }

    return QDir::cleanPath(normalized_path).toStdWString();
}

std::wstring LibraryManager::normalize_collection_name(const std::wstring& collection_name) {
    return QString::fromStdWString(collection_name).trimmed().toStdWString();
}

bool LibraryManager::is_pdf_file(const std::wstring& path) {
    QFileInfo file_info(QString::fromStdWString(path));
    return file_info.suffix().compare("pdf", Qt::CaseInsensitive) == 0;
}
