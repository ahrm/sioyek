#include <cstdlib>
#include <iostream>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include "workspace_manager.h"

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "workspace_manager_test failed: " << message << "\n";
        std::exit(1);
    }
}

WorkspaceDocumentEntry make_document(const std::wstring& path, int page, float offset_y) {
    WorkspaceDocumentEntry entry;
    entry.path = path;
    entry.display_name = L"";
    entry.page = page;
    entry.zoom_level = 1.25f;
    entry.offset_x = 12.0f;
    entry.offset_y = offset_y;
    entry.window_index = page;
    return entry;
}

}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    QTemporaryDir temp_dir;
    expect(temp_dir.isValid(), "temporary directory is valid");

    const std::wstring workspace_path = QDir(temp_dir.path()).filePath("sioyek_workspaces.json").toStdWString();
    const std::wstring pdf_path = QDir(temp_dir.path()).filePath("Paper.pdf").toStdWString();
    const std::wstring second_pdf_path = QDir(temp_dir.path()).filePath("Second.pdf").toStdWString();

    for (const std::wstring& path : { pdf_path, second_pdf_path }) {
        QFile pdf_file(QString::fromStdWString(path));
        expect(pdf_file.open(QIODevice::WriteOnly | QIODevice::Truncate), "created temporary pdf file");
        pdf_file.close();
    }

    WorkspaceManager manager(workspace_path);
    expect(manager.load(), "empty workspace file loads");
    expect(manager.workspaces().empty(), "fresh workspace list is empty");
    expect(manager.save_workspace(L"  ", { make_document(pdf_path, 3, 100.0f) }) == WorkspaceSaveResult::InvalidName, "blank workspace name is rejected");
    expect(manager.save_workspace(L"Research", {}) == WorkspaceSaveResult::EmptyWorkspace, "empty workspace is rejected");

    std::vector<WorkspaceDocumentEntry> documents;
    documents.push_back(make_document(pdf_path, 3, 100.0f));
    documents.push_back(make_document(pdf_path, 9, 900.0f));
    documents.push_back(make_document(second_pdf_path, 6, 600.0f));
    expect(manager.save_workspace(L"Research", documents) == WorkspaceSaveResult::Saved, "workspace saves");
    expect(manager.workspace_exists(L"Research"), "saved workspace exists");
    expect(manager.active_workspace().has_value() && manager.active_workspace().value() == L"Research", "saved workspace becomes active");

    std::optional<WorkspaceEntry> workspace = manager.get_workspace(L"Research");
    expect(workspace.has_value(), "saved workspace can be read");
    expect(workspace->documents.size() == 2, "duplicate document paths are removed");
    expect(workspace->documents[0].display_name == L"Paper.pdf", "display name defaults to basename");
    expect(workspace->documents[0].page == 3, "page is stored");
    expect(workspace->documents[0].zoom_level == 1.25f, "zoom is stored");
    expect(workspace->documents[0].offset_x == 12.0f, "x offset is stored");
    expect(workspace->documents[0].offset_y == 100.0f, "y offset is stored");
    expect(workspace->documents[0].window_index.has_value(), "window index is stored");
    expect(!workspace->documents[0].last_saved.empty(), "document last_saved is stored");
    expect(!workspace->created_at.empty(), "created_at is stored");
    expect(!workspace->updated_at.empty(), "updated_at is stored");

    WorkspaceManager reloaded(workspace_path);
    expect(reloaded.load(), "saved workspaces reload");
    expect(reloaded.workspaces().size() == 1, "one workspace reloads");
    expect(reloaded.get_workspace(L"Research").has_value(), "workspace lookup works after reload");
    expect(reloaded.get_workspace(L"Research")->documents.size() == 2, "workspace documents reload");
    expect(reloaded.get_active_workspace().has_value() && reloaded.get_active_workspace().value() == L"Research", "active workspace reloads");

    expect(reloaded.save_workspace(L"Research", { make_document(second_pdf_path, 12, 1200.0f) }) == WorkspaceSaveResult::Saved, "workspace update saves");
    expect(reloaded.workspaces().size() == 1, "workspace update does not duplicate name");
    expect(reloaded.get_workspace(L"Research")->documents.size() == 1, "workspace update replaces document set");
    expect(reloaded.get_workspace(L"Research")->documents[0].page == 12, "workspace update stores new page");

    expect(reloaded.save_workspace(L"Reading", { make_document(pdf_path, 2, 200.0f) }) == WorkspaceSaveResult::Saved, "second workspace saves");
    expect(reloaded.workspaces().size() == 2, "second workspace is listed");
    expect(reloaded.list_workspaces().size() == 2, "list_workspaces exposes saved workspaces");
    expect(reloaded.workspace_contains_document(L"Research", second_pdf_path), "workspace membership detects saved document");

    WorkspaceDocumentEntry updated_current_doc = make_document(pdf_path, 22, 2200.0f);
    expect(reloaded.add_or_update_document(L"Research", updated_current_doc) == WorkspaceDocumentChangeResult::Added, "document add succeeds");
    expect(reloaded.get_workspace(L"Research")->documents.size() == 2, "document add appends without replacing workspace");
    expect(reloaded.add_or_update_document(L"Research", make_document(pdf_path, 23, 2300.0f)) == WorkspaceDocumentChangeResult::Updated, "document update succeeds");
    expect(reloaded.get_workspace(L"Research")->documents.size() == 2, "document update does not duplicate path");
    expect(reloaded.get_workspace(L"Research")->documents[1].page == 23, "document update stores new state");
    expect(reloaded.workspaces_containing_document(pdf_path).size() == 2, "containing workspaces lists all matches");

    WorkspacePathValidationResult validation = reloaded.validate_workspace_paths(L"Research");
    expect(validation.documents == 2, "path validation counts documents");
    expect(validation.missing == 0, "path validation sees existing documents");

    const std::wstring missing_pdf_path = QDir(temp_dir.path()).filePath("Missing.pdf").toStdWString();
    expect(reloaded.add_or_update_document(L"Research", make_document(missing_pdf_path, 4, 400.0f)) == WorkspaceDocumentChangeResult::Added, "missing document path can be stored");
    validation = reloaded.validate_workspace_paths(L"Research");
    expect(validation.documents == 3, "path validation includes missing document");
    expect(validation.missing == 1, "path validation counts missing document");
    expect(validation.missing_paths.size() == 1 && validation.missing_paths[0] == missing_pdf_path, "path validation returns missing path");

    reloaded.set_active_workspace(L"Research");
    expect(reloaded.rename_workspace(L"Research", L"Renamed Research") == WorkspaceRenameResult::Renamed, "workspace rename succeeds");
    expect(!reloaded.workspace_exists(L"Research"), "old workspace name removed after rename");
    expect(reloaded.workspace_exists(L"Renamed Research"), "new workspace name exists after rename");
    expect(reloaded.get_workspace(L"Renamed Research")->documents.size() == 3, "rename preserves documents");
    expect(reloaded.get_active_workspace().has_value() && reloaded.get_active_workspace().value() == L"Renamed Research", "rename updates active workspace");
    expect(reloaded.rename_workspace(L"Renamed Research", L"Reading") == WorkspaceRenameResult::AlreadyExists, "rename rejects duplicate names");
    expect(reloaded.rename_workspace(L"Missing", L"Anything") == WorkspaceRenameResult::NotFound, "rename reports missing workspace");
    expect(reloaded.remove_document_from_workspace(L"Renamed Research", missing_pdf_path) == WorkspaceDocumentChangeResult::Removed, "document remove succeeds");
    expect(!reloaded.workspace_contains_document(L"Renamed Research", missing_pdf_path), "document remove updates membership");
    expect(QFile::exists(QString::fromStdWString(pdf_path)), "document remove does not delete PDFs");

    expect(reloaded.delete_workspace(L"Renamed Research") == WorkspaceDeleteResult::Deleted, "renamed workspace delete succeeds");
    expect(!reloaded.workspace_exists(L"Renamed Research"), "deleted workspace is removed");
    expect(reloaded.workspace_exists(L"Reading"), "other workspace remains");
    expect(QFile::exists(QString::fromStdWString(second_pdf_path)), "workspace delete does not delete PDFs");
    expect(reloaded.delete_workspace(L"Research") == WorkspaceDeleteResult::NotFound, "missing workspace delete is reported");

    WorkspaceManager deleted_reloaded(workspace_path);
    expect(deleted_reloaded.load(), "deleted workspace file reloads");
    expect(deleted_reloaded.workspaces().size() == 1, "workspace delete persists");
    expect(deleted_reloaded.workspace_exists(L"Reading"), "remaining workspace persists");

    const std::wstring old_workspace_path = QDir(temp_dir.path()).filePath("old_sioyek_workspaces.json").toStdWString();
    QJsonObject old_document;
    old_document["path"] = QString::fromStdWString(pdf_path);
    old_document["page"] = 7;
    old_document["zoom"] = 1.5;
    old_document["offset_x"] = 22.0;
    old_document["offset_y"] = 700.0;
    QJsonArray old_documents;
    old_documents.append(old_document);
    QJsonObject old_workspace;
    old_workspace["name"] = "Old Workspace";
    old_workspace["documents"] = old_documents;
    QJsonArray old_workspaces;
    old_workspaces.append(old_workspace);
    QJsonObject old_root;
    old_root["version"] = 1;
    old_root["workspaces"] = old_workspaces;

    QFile old_workspace_file(QString::fromStdWString(old_workspace_path));
    expect(old_workspace_file.open(QIODevice::WriteOnly | QIODevice::Truncate), "old workspace file opens");
    old_workspace_file.write(QJsonDocument(old_root).toJson());
    old_workspace_file.close();

    WorkspaceManager old_manager(old_workspace_path);
    expect(old_manager.load(), "old workspace file loads");
    expect(old_manager.workspace_exists(L"Old Workspace"), "old workspace name loads");
    expect(old_manager.get_workspace(L"Old Workspace")->documents[0].display_name == L"Paper.pdf", "old workspace display name is inferred");
    expect(!old_manager.get_workspace(L"Old Workspace")->documents[0].last_saved.empty(), "old workspace last_saved is inferred");

    QFile invalid_json(QString::fromStdWString(workspace_path));
    expect(invalid_json.open(QIODevice::WriteOnly | QIODevice::Truncate), "invalid json file opens");
    invalid_json.write("{ invalid json");
    invalid_json.close();

    WorkspaceManager invalid_manager(workspace_path);
    expect(!invalid_manager.load(), "invalid json load fails cleanly");
    expect(invalid_manager.workspaces().empty(), "invalid json leaves empty workspace list");

    std::cout << "workspace_manager_test passed\n";
    return 0;
}
