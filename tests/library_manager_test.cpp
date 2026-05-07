#include <iostream>
#include <cstdlib>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include "library_manager.h"

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "library_manager_test failed: " << message << "\n";
        std::exit(1);
    }
}

}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    QTemporaryDir temp_dir;
    expect(temp_dir.isValid(), "temporary directory is valid");

    const std::wstring library_path = QDir(temp_dir.path()).filePath("sioyek_library.json").toStdWString();
    const std::wstring pdf_path = QDir(temp_dir.path()).filePath("Paper.pdf").toStdWString();

    QFile pdf_file(QString::fromStdWString(pdf_path));
    expect(pdf_file.open(QIODevice::WriteOnly), "created temporary pdf file");
    pdf_file.close();

    LibraryManager manager(library_path);
    expect(manager.load(), "empty library loads");
    expect(manager.list_collections().empty(), "fresh library has no collections");

    expect(manager.add_document(pdf_path) == LibraryAddResult::Added, "first add succeeds");
    expect(manager.add_document(pdf_path) == LibraryAddResult::AlreadyExists, "duplicate add is rejected");
    expect(manager.entries().size() == 1, "duplicate add keeps one entry");
    expect(manager.entries()[0].display_name == L"Paper.pdf", "display name defaults to basename");
    expect(manager.entries()[0].tags.empty(), "tags default to empty list");
    expect(manager.entries()[0].collections.empty(), "collections default to empty list");
    expect(!manager.entries()[0].date_added.empty(), "date_added is stored");
    expect(!manager.entries()[0].last_opened.has_value(), "last_opened starts empty");
    expect(manager.uncategorized_documents().size() == 1, "document starts uncategorized");

    expect(manager.create_collection(L"Research") == LibraryCreateCollectionResult::Created, "collection create succeeds");
    expect(manager.create_collection(L"Research") == LibraryCreateCollectionResult::AlreadyExists, "duplicate collection create is rejected");
    expect(manager.create_collection(L"  ") == LibraryCreateCollectionResult::InvalidName, "blank collection create is rejected");
    expect(manager.collection_exists(L"Research"), "created collection exists");
    expect(manager.list_collections().size() == 1, "created collection is listed");

    expect(manager.add_document_to_collection(pdf_path, L"Research") == LibraryAddToCollectionResult::Added, "document added to collection");
    expect(manager.add_document_to_collection(pdf_path, L"Research") == LibraryAddToCollectionResult::AlreadyInCollection, "duplicate document collection is rejected");
    expect(manager.add_document_to_collection(pdf_path, L"Reading List") == LibraryAddToCollectionResult::Added, "document can be added to second collection");
    expect(manager.collection_exists(L"Reading List"), "new collection is created when adding document to it");
    expect(manager.entries()[0].collections.size() == 2, "document records multiple collections");
    expect(manager.documents_in_collection(L"Research").size() == 1, "documents_in_collection finds member");
    expect(manager.uncategorized_documents().empty(), "categorized document is not uncategorized");

    LibraryManager reloaded(library_path);
    expect(reloaded.load(), "saved library reloads");
    expect(reloaded.entries().size() == 1, "saved library has one entry");
    expect(reloaded.entries()[0].path == manager.entries()[0].path, "path survives reload");
    expect(reloaded.collection_exists(L"Research"), "collection survives reload");
    expect(reloaded.collection_exists(L"Reading List"), "auto-created collection survives reload");
    expect(reloaded.entries()[0].collections.size() == 2, "document collections survive reload");
    expect(reloaded.update_last_opened(pdf_path), "last_opened update saves");
    expect(reloaded.entries()[0].last_opened.has_value(), "last_opened is set");
    expect(reloaded.remove_document_from_collection(pdf_path, L"Research") == LibraryRemoveFromCollectionResult::Removed, "document removed from collection");
    expect(reloaded.remove_document_from_collection(pdf_path, L"Research") == LibraryRemoveFromCollectionResult::NotInCollection, "missing document collection removal is reported");
    expect(reloaded.documents_in_collection(L"Research").empty(), "removed collection has no documents");
    expect(reloaded.documents_in_collection(L"Reading List").size() == 1, "other collection remains");

    LibraryManager opened_reloaded(library_path);
    expect(opened_reloaded.load(), "last_opened library reloads");
    expect(opened_reloaded.entries().size() == 1, "last_opened reload has one entry");
    expect(opened_reloaded.entries()[0].last_opened.has_value(), "last_opened survives reload");
    expect(opened_reloaded.collection_exists(L"Research"), "empty collection persists");
    expect(opened_reloaded.collection_exists(L"Reading List"), "non-empty collection persists");

    expect(opened_reloaded.remove_document(pdf_path) == LibraryRemoveResult::Removed, "remove succeeds");

    LibraryManager removed_reloaded(library_path);
    expect(removed_reloaded.load(), "removed library reloads");
    expect(removed_reloaded.entries().empty(), "remove persists");
    expect(removed_reloaded.collection_exists(L"Research"), "empty collection remains after document removal");

    const std::wstring old_library_path = QDir(temp_dir.path()).filePath("old_sioyek_library.json").toStdWString();
    QJsonObject old_entry;
    old_entry["path"] = QString::fromStdWString(pdf_path);
    old_entry["display_name"] = "Paper.pdf";
    old_entry["date_added"] = "2026-04-29T00:00:00.000Z";
    old_entry["tags"] = QJsonArray();
    QJsonArray old_entry_collections;
    old_entry_collections.append("Imported");
    old_entry["collections"] = old_entry_collections;
    QJsonArray old_entries;
    old_entries.append(old_entry);
    QJsonObject old_root;
    old_root["version"] = 1;
    old_root["entries"] = old_entries;

    QFile old_library_file(QString::fromStdWString(old_library_path));
    expect(old_library_file.open(QIODevice::WriteOnly | QIODevice::Truncate), "old library file opens");
    old_library_file.write(QJsonDocument(old_root).toJson());
    old_library_file.close();

    LibraryManager old_manager(old_library_path);
    expect(old_manager.load(), "old library without top-level collections loads");
    expect(old_manager.collection_exists(L"Imported"), "old entry collections are promoted to collection list");
    expect(old_manager.documents_in_collection(L"Imported").size() == 1, "old entry collection membership works");

    const std::wstring import_library_path = QDir(temp_dir.path()).filePath("import_sioyek_library.json").toStdWString();
    QDir temp_qdir(temp_dir.path());
    expect(temp_qdir.mkpath("imports/nested/deeper"), "created import folders");
    const QString import_root = temp_qdir.filePath("imports");
    const std::wstring import_pdf_1 = QDir(import_root).filePath("One.pdf").toStdWString();
    const std::wstring import_pdf_2 = QDir(import_root).filePath("Two.PDF").toStdWString();
    const std::wstring import_text = QDir(import_root).filePath("notes.txt").toStdWString();
    const std::wstring nested_pdf = QDir(import_root).filePath("nested/Three.pdf").toStdWString();
    const std::wstring deeper_nested_pdf = QDir(import_root).filePath("nested/deeper/Four.pdf").toStdWString();

    for (const std::wstring& path : { import_pdf_1, import_pdf_2, import_text, nested_pdf, deeper_nested_pdf }) {
        QFile file(QString::fromStdWString(path));
        expect(file.open(QIODevice::WriteOnly | QIODevice::Truncate), "created import test file");
        file.close();
    }

    LibraryManager import_manager(import_library_path);
    expect(import_manager.load(), "import library loads");
    expect(import_manager.import_file(L"") == LibraryImportFileResult::InvalidPath, "import_file rejects empty path");
    expect(import_manager.import_file(import_root.toStdWString()) == LibraryImportFileResult::NotAFile, "import_file rejects directories");
    expect(import_manager.import_file(import_text) == LibraryImportFileResult::NotPdf, "import_file rejects non-pdf");
    expect(import_manager.import_file(QDir(import_root).filePath("missing.pdf").toStdWString()) == LibraryImportFileResult::FileNotFound, "import_file reports missing file");
    expect(import_manager.import_file(import_pdf_1) == LibraryImportFileResult::Added, "import_file adds pdf");
    expect(import_manager.import_file(import_pdf_1) == LibraryImportFileResult::AlreadyExists, "import_file skips duplicate pdf");
    expect(import_manager.is_document_in_library(import_pdf_1), "is_document_in_library finds imported pdf");
    expect(import_manager.get_document_info(import_pdf_1).has_value(), "get_document_info returns imported entry");

    LibraryImportSummary missing_folder_summary = import_manager.import_folder(QDir(import_root).filePath("missing").toStdWString());
    expect(missing_folder_summary.source_status == LibraryImportSourceStatus::NotFound, "import_folder reports missing folder");

    LibraryImportSummary folder_summary = import_manager.import_folder(import_root.toStdWString());
    expect(folder_summary.source_status == LibraryImportSourceStatus::Ok, "import_folder accepts import folder");
    expect(folder_summary.files_found == 2, "non-recursive import finds root pdfs only");
    expect(folder_summary.files_added == 1, "non-recursive import adds new root pdf");
    expect(folder_summary.files_already_present == 1, "non-recursive import skips existing pdf");
    expect(folder_summary.files_skipped_not_pdf == 1, "non-recursive import ignores non-pdf");
    expect(import_manager.entries().size() == 2, "non-recursive import does not add nested pdf");

    LibraryImportSummary recursive_summary = import_manager.import_folder(import_root.toStdWString(), true);
    expect(recursive_summary.source_status == LibraryImportSourceStatus::Ok, "recursive import accepts import folder");
    expect(recursive_summary.files_found == 4, "recursive import finds nested pdfs");
    expect(recursive_summary.files_added == 2, "recursive import adds nested pdfs");
    expect(recursive_summary.files_already_present == 2, "recursive import skips duplicate root pdfs");
    expect(recursive_summary.files_added_to_collection == 2, "recursive import adds nested pdfs to relative collections");
    expect(recursive_summary.collections_created == 2, "recursive import creates one collection per relative subfolder");
    expect(import_manager.entries().size() == 4, "recursive import adds all pdfs");
    expect(import_manager.documents_in_collection(L"nested").size() == 1, "recursive import uses relative subfolder collection");
    expect(import_manager.documents_in_collection(L"nested-deeper").size() == 1, "recursive import replaces nested path separators with hyphens");
    expect(!import_manager.collection_exists(L"nested/deeper"), "recursive import does not create slash-separated collection names");

    LibraryImportSummary collection_summary = import_manager.import_folder_to_collection(import_root.toStdWString(), L"Imported PDFs", false);
    expect(collection_summary.source_status == LibraryImportSourceStatus::Ok, "folder-to-collection import succeeds");
    expect(collection_summary.collection_created, "folder-to-collection creates collection");
    expect(collection_summary.files_found == 2, "folder-to-collection uses non-recursive scan");
    expect(collection_summary.files_added == 0, "folder-to-collection skips existing files");
    expect(collection_summary.files_already_present == 2, "folder-to-collection counts existing files");
    expect(collection_summary.files_added_to_collection == 2, "folder-to-collection adds existing files to collection");
    expect(import_manager.documents_in_collection(L"Imported PDFs").size() == 2, "folder-to-collection membership is stored");

    LibraryImportSummary duplicate_collection_summary = import_manager.import_folder_to_collection(import_root.toStdWString(), L"Imported PDFs", false);
    expect(!duplicate_collection_summary.collection_created, "duplicate folder-to-collection does not recreate collection");
    expect(duplicate_collection_summary.files_already_in_collection == 2, "duplicate folder-to-collection skips duplicate memberships");
    expect(import_manager.documents_in_collection(L"Imported PDFs").size() == 2, "duplicate folder-to-collection keeps unique memberships");

    LibraryImportSummary recursive_collection_summary = import_manager.import_folder_to_collection(import_root.toStdWString(), L"All PDFs", true);
    expect(recursive_collection_summary.source_status == LibraryImportSourceStatus::Ok, "recursive folder-to-collection import succeeds");
    expect(recursive_collection_summary.files_found == 4, "recursive folder-to-collection scans all pdfs");
    expect(recursive_collection_summary.files_added == 0, "recursive folder-to-collection skips existing files");
    expect(recursive_collection_summary.files_already_present == 4, "recursive folder-to-collection counts existing files");
    expect(recursive_collection_summary.files_added_to_collection == 4, "recursive folder-to-collection adds all pdfs to collections");
    expect(recursive_collection_summary.collections_created == 3, "recursive folder-to-collection creates root and relative collections");
    expect(import_manager.documents_in_collection(L"All PDFs").size() == 2, "recursive folder-to-collection keeps root pdfs in chosen collection");
    expect(import_manager.documents_in_collection(L"All PDFs-nested").size() == 1, "recursive folder-to-collection prefixes first-level relative collection");
    expect(import_manager.documents_in_collection(L"All PDFs-nested-deeper").size() == 1, "recursive folder-to-collection prefixes nested relative collection");
    expect(!import_manager.collection_exists(L"All PDFs/nested/deeper"), "recursive folder-to-collection avoids slash-separated collection names");

    QFile invalid_json(QString::fromStdWString(library_path));
    expect(invalid_json.open(QIODevice::WriteOnly | QIODevice::Truncate), "invalid json file opens");
    invalid_json.write("{ invalid json");
    invalid_json.close();

    LibraryManager invalid_manager(library_path);
    expect(!invalid_manager.load(), "invalid json load fails cleanly");
    expect(invalid_manager.entries().empty(), "invalid json leaves empty library");

    std::cout << "library_manager_test passed\n";
    return 0;
}
