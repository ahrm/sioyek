#include "workspace_manager.h"

#include <algorithm>
#include <set>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSaveFile>
#include <QString>

#include "library_manager.h"

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

QJsonObject document_entry_to_json(const WorkspaceDocumentEntry& entry) {
    QJsonObject object;
    object["path"] = QString::fromStdWString(entry.path);
    object["display_name"] = QString::fromStdWString(entry.display_name);
    object["page"] = entry.page;
    object["zoom"] = entry.zoom_level;
    object["offset_x"] = entry.offset_x;
    object["offset_y"] = entry.offset_y;
    object["last_saved"] = QString::fromStdString(entry.last_saved);
    if (entry.window_index.has_value()) {
        object["window_index"] = entry.window_index.value();
    }
    else {
        object["window_index"] = QJsonValue::Null;
    }
    object["crop"] = QJsonValue::Null;
    return object;
}

std::optional<WorkspaceDocumentEntry> document_entry_from_json(const QJsonValue& value) {
    if (!value.isObject()) {
        return {};
    }

    QJsonObject object = value.toObject();
    if (!object.value("path").isString()) {
        return {};
    }

    std::wstring path = WorkspaceManager::normalize_workspace_path(object.value("path").toString().toStdWString());
    if (path.empty()) {
        return {};
    }

    WorkspaceDocumentEntry entry;
    entry.path = path;
    entry.display_name = object.value("display_name").isString()
        ? object.value("display_name").toString().toStdWString()
        : default_display_name(path);
    if (entry.display_name.empty()) {
        entry.display_name = default_display_name(path);
    }
    entry.page = object.value("page").isDouble() ? object.value("page").toInt() : -1;
    entry.zoom_level = object.value("zoom").isDouble() ? static_cast<float>(object.value("zoom").toDouble()) : 0.0f;
    entry.offset_x = object.value("offset_x").isDouble() ? static_cast<float>(object.value("offset_x").toDouble()) : 0.0f;
    entry.offset_y = object.value("offset_y").isDouble() ? static_cast<float>(object.value("offset_y").toDouble()) : 0.0f;
    if (object.value("window_index").isDouble()) {
        entry.window_index = object.value("window_index").toInt();
    }
    entry.last_saved = object.value("last_saved").isString()
        ? object.value("last_saved").toString().toStdString()
        : current_timestamp();
    return entry;
}

QJsonObject workspace_to_json(const WorkspaceEntry& workspace) {
    QJsonObject object;
    object["name"] = QString::fromStdWString(workspace.name);
    object["created_at"] = QString::fromStdString(workspace.created_at);
    object["updated_at"] = QString::fromStdString(workspace.updated_at);

    QJsonArray documents_array;
    for (const WorkspaceDocumentEntry& document : workspace.documents) {
        documents_array.append(document_entry_to_json(document));
    }
    object["documents"] = documents_array;
    return object;
}

std::optional<WorkspaceEntry> workspace_from_json(const QJsonValue& value) {
    if (!value.isObject()) {
        return {};
    }

    QJsonObject object = value.toObject();
    if (!object.value("name").isString()) {
        return {};
    }

    WorkspaceEntry workspace;
    workspace.name = WorkspaceManager::normalize_workspace_name(object.value("name").toString().toStdWString());
    if (workspace.name.empty()) {
        return {};
    }
    workspace.created_at = object.value("created_at").isString()
        ? object.value("created_at").toString().toStdString()
        : current_timestamp();
    workspace.updated_at = object.value("updated_at").isString()
        ? object.value("updated_at").toString().toStdString()
        : workspace.created_at;

    std::set<std::wstring> seen_paths;
    if (object.value("documents").isArray()) {
        for (const QJsonValue& document_value : object.value("documents").toArray()) {
            std::optional<WorkspaceDocumentEntry> document = document_entry_from_json(document_value);
            if (document.has_value() && seen_paths.find(document->path) == seen_paths.end()) {
                seen_paths.insert(document->path);
                workspace.documents.push_back(document.value());
            }
        }
    }

    return workspace;
}

WorkspaceDocumentEntry normalized_document_entry(WorkspaceDocumentEntry document, const std::string& timestamp) {
    document.path = WorkspaceManager::normalize_workspace_path(document.path);
    if (document.path.empty()) {
        return document;
    }
    if (document.display_name.empty()) {
        document.display_name = default_display_name(document.path);
    }
    document.last_saved = timestamp;
    return document;
}

}

WorkspaceManager::WorkspaceManager(const std::wstring& workspace_file_path_) : workspace_file_path(workspace_file_path_) {
}

std::vector<WorkspaceEntry>::iterator WorkspaceManager::find_workspace(const std::wstring& workspace_name) {
    return std::find_if(workspace_entries.begin(), workspace_entries.end(), [&](const WorkspaceEntry& workspace) {
        return workspace.name == workspace_name;
        });
}

std::vector<WorkspaceEntry>::const_iterator WorkspaceManager::find_workspace(const std::wstring& workspace_name) const {
    return std::find_if(workspace_entries.begin(), workspace_entries.end(), [&](const WorkspaceEntry& workspace) {
        return workspace.name == workspace_name;
        });
}

void WorkspaceManager::set_last_error(const std::wstring& message) {
    last_error_message = message;
}

bool WorkspaceManager::load() {
    workspace_entries.clear();
    last_error_message.clear();
    active_workspace_name = {};

    QFile file(QString::fromStdWString(workspace_file_path));
    if (!file.exists()) {
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        set_last_error(L"Could not read workspace file: " + workspace_file_path);
        return false;
    }

    QJsonParseError parse_error;
    QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
        set_last_error(L"Could not parse workspace file: " + parse_error.errorString().toStdWString());
        workspace_entries.clear();
        return false;
    }

    QJsonObject root = document.object();
    QJsonValue workspaces_value = root.value("workspaces");
    if (!workspaces_value.isArray()) {
        set_last_error(L"Workspace file does not contain a workspaces array");
        workspace_entries.clear();
        return false;
    }

    std::set<std::wstring> seen_names;
    for (const QJsonValue& value : workspaces_value.toArray()) {
        std::optional<WorkspaceEntry> workspace = workspace_from_json(value);
        if (workspace.has_value() && seen_names.find(workspace->name) == seen_names.end()) {
            seen_names.insert(workspace->name);
            workspace_entries.push_back(workspace.value());
        }
    }

    if (root.value("active_workspace").isString()) {
        std::wstring active_name = normalize_workspace_name(root.value("active_workspace").toString().toStdWString());
        if (!active_name.empty() && find_workspace(active_name) != workspace_entries.end()) {
            active_workspace_name = active_name;
        }
    }

    return true;
}

bool WorkspaceManager::save() {
    last_error_message.clear();

    QJsonObject root;
    root["version"] = 1;
    if (active_workspace_name.has_value()) {
        root["active_workspace"] = QString::fromStdWString(active_workspace_name.value());
    }

    QJsonArray workspaces_array;
    for (const WorkspaceEntry& workspace : workspace_entries) {
        workspaces_array.append(workspace_to_json(workspace));
    }
    root["workspaces"] = workspaces_array;

    QFileInfo file_info(QString::fromStdWString(workspace_file_path));
    if (!file_info.dir().exists() && !file_info.dir().mkpath(".")) {
        set_last_error(L"Could not create workspace directory: " + file_info.dir().absolutePath().toStdWString());
        return false;
    }

    QSaveFile file(QString::fromStdWString(workspace_file_path));
    if (!file.open(QIODevice::WriteOnly)) {
        set_last_error(L"Could not write workspace file: " + workspace_file_path);
        return false;
    }

    QJsonDocument document(root);
    file.write(document.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        set_last_error(L"Could not commit workspace file: " + workspace_file_path);
        return false;
    }
    return true;
}

WorkspaceSaveResult WorkspaceManager::save_workspace(const WorkspaceEntry& workspace) {
    return save_workspace(workspace.name, workspace.documents);
}

WorkspaceSaveResult WorkspaceManager::save_workspace(const std::wstring& workspace_name, const std::vector<WorkspaceDocumentEntry>& documents) {
    std::wstring normalized_name = normalize_workspace_name(workspace_name);
    if (normalized_name.empty()) {
        return WorkspaceSaveResult::InvalidName;
    }
    if (documents.empty()) {
        return WorkspaceSaveResult::EmptyWorkspace;
    }

    std::string timestamp = current_timestamp();
    WorkspaceEntry next_workspace;
    next_workspace.name = normalized_name;
    next_workspace.created_at = timestamp;
    next_workspace.updated_at = timestamp;

    std::set<std::wstring> seen_paths;
    for (WorkspaceDocumentEntry document : documents) {
        document = normalized_document_entry(document, timestamp);
        if (document.path.empty() || seen_paths.find(document.path) != seen_paths.end()) {
            continue;
        }
        seen_paths.insert(document.path);
        next_workspace.documents.push_back(document);
    }

    if (next_workspace.documents.empty()) {
        return WorkspaceSaveResult::EmptyWorkspace;
    }

    std::optional<std::wstring> previous_active_workspace = active_workspace_name;
    active_workspace_name = normalized_name;

    auto workspace_it = find_workspace(normalized_name);
    if (workspace_it != workspace_entries.end()) {
        next_workspace.created_at = workspace_it->created_at;
        WorkspaceEntry previous_workspace = *workspace_it;
        *workspace_it = next_workspace;
        if (!save()) {
            *workspace_it = previous_workspace;
            active_workspace_name = previous_active_workspace;
            return WorkspaceSaveResult::SaveFailed;
        }
    }
    else {
        workspace_entries.push_back(next_workspace);
        if (!save()) {
            workspace_entries.pop_back();
            active_workspace_name = previous_active_workspace;
            return WorkspaceSaveResult::SaveFailed;
        }
    }

    return WorkspaceSaveResult::Saved;
}

WorkspaceSaveResult WorkspaceManager::update_workspace_from_current_session(const std::wstring& workspace_name, const std::vector<WorkspaceDocumentEntry>& documents) {
    std::wstring normalized_name = normalize_workspace_name(workspace_name);
    if (normalized_name.empty()) {
        return WorkspaceSaveResult::InvalidName;
    }
    if (find_workspace(normalized_name) == workspace_entries.end()) {
        return WorkspaceSaveResult::InvalidName;
    }
    return save_workspace(normalized_name, documents);
}

WorkspaceDeleteResult WorkspaceManager::delete_workspace(const std::wstring& workspace_name) {
    std::wstring normalized_name = normalize_workspace_name(workspace_name);
    if (normalized_name.empty()) {
        return WorkspaceDeleteResult::InvalidName;
    }

    auto workspace_it = find_workspace(normalized_name);
    if (workspace_it == workspace_entries.end()) {
        return WorkspaceDeleteResult::NotFound;
    }

    WorkspaceEntry removed_workspace = *workspace_it;
    std::optional<std::wstring> previous_active_workspace = active_workspace_name;
    int removed_index = static_cast<int>(workspace_it - workspace_entries.begin());
    workspace_entries.erase(workspace_it);
    if (active_workspace_name.has_value() && active_workspace_name.value() == normalized_name) {
        active_workspace_name = {};
    }
    if (!save()) {
        workspace_entries.insert(workspace_entries.begin() + removed_index, removed_workspace);
        active_workspace_name = previous_active_workspace;
        return WorkspaceDeleteResult::SaveFailed;
    }

    return WorkspaceDeleteResult::Deleted;
}

WorkspaceRenameResult WorkspaceManager::rename_workspace(const std::wstring& old_name, const std::wstring& new_name) {
    std::wstring normalized_old_name = normalize_workspace_name(old_name);
    std::wstring normalized_new_name = normalize_workspace_name(new_name);
    if (normalized_old_name.empty() || normalized_new_name.empty()) {
        return WorkspaceRenameResult::InvalidName;
    }

    auto old_workspace_it = find_workspace(normalized_old_name);
    if (old_workspace_it == workspace_entries.end()) {
        return WorkspaceRenameResult::NotFound;
    }
    if (normalized_old_name != normalized_new_name && find_workspace(normalized_new_name) != workspace_entries.end()) {
        return WorkspaceRenameResult::AlreadyExists;
    }

    WorkspaceEntry previous_workspace = *old_workspace_it;
    std::optional<std::wstring> previous_active_workspace = active_workspace_name;
    old_workspace_it->name = normalized_new_name;
    old_workspace_it->updated_at = current_timestamp();
    if (active_workspace_name.has_value() && active_workspace_name.value() == normalized_old_name) {
        active_workspace_name = normalized_new_name;
    }

    if (!save()) {
        *old_workspace_it = previous_workspace;
        active_workspace_name = previous_active_workspace;
        return WorkspaceRenameResult::SaveFailed;
    }
    return WorkspaceRenameResult::Renamed;
}

WorkspaceDocumentChangeResult WorkspaceManager::add_or_update_document(const std::wstring& workspace_name, const WorkspaceDocumentEntry& document) {
    std::wstring normalized_name = normalize_workspace_name(workspace_name);
    if (normalized_name.empty()) {
        return WorkspaceDocumentChangeResult::InvalidWorkspaceName;
    }

    auto workspace_it = find_workspace(normalized_name);
    if (workspace_it == workspace_entries.end()) {
        return WorkspaceDocumentChangeResult::WorkspaceNotFound;
    }

    std::string timestamp = current_timestamp();
    WorkspaceDocumentEntry normalized_document = normalized_document_entry(document, timestamp);
    if (normalized_document.path.empty()) {
        return WorkspaceDocumentChangeResult::InvalidPath;
    }

    WorkspaceEntry previous_workspace = *workspace_it;
    auto document_it = std::find_if(workspace_it->documents.begin(), workspace_it->documents.end(), [&](const WorkspaceDocumentEntry& entry) {
        return entry.path == normalized_document.path;
        });
    WorkspaceDocumentChangeResult result = WorkspaceDocumentChangeResult::Added;
    if (document_it == workspace_it->documents.end()) {
        workspace_it->documents.push_back(normalized_document);
    }
    else {
        *document_it = normalized_document;
        result = WorkspaceDocumentChangeResult::Updated;
    }
    workspace_it->updated_at = timestamp;

    if (!save()) {
        *workspace_it = previous_workspace;
        return WorkspaceDocumentChangeResult::SaveFailed;
    }
    return result;
}

WorkspaceDocumentChangeResult WorkspaceManager::add_current_document_to_workspace(const std::wstring& workspace_name, const WorkspaceDocumentEntry& document) {
    return add_or_update_document(workspace_name, document);
}

WorkspaceDocumentChangeResult WorkspaceManager::remove_document_from_workspace(const std::wstring& workspace_name, const std::wstring& path) {
    std::wstring normalized_name = normalize_workspace_name(workspace_name);
    if (normalized_name.empty()) {
        return WorkspaceDocumentChangeResult::InvalidWorkspaceName;
    }

    std::wstring normalized_path = normalize_workspace_path(path);
    if (normalized_path.empty()) {
        return WorkspaceDocumentChangeResult::InvalidPath;
    }

    auto workspace_it = find_workspace(normalized_name);
    if (workspace_it == workspace_entries.end()) {
        return WorkspaceDocumentChangeResult::WorkspaceNotFound;
    }

    auto document_it = std::find_if(workspace_it->documents.begin(), workspace_it->documents.end(), [&](const WorkspaceDocumentEntry& entry) {
        return entry.path == normalized_path;
        });
    if (document_it == workspace_it->documents.end()) {
        return WorkspaceDocumentChangeResult::DocumentNotFound;
    }

    WorkspaceEntry previous_workspace = *workspace_it;
    workspace_it->documents.erase(document_it);
    workspace_it->updated_at = current_timestamp();
    if (!save()) {
        *workspace_it = previous_workspace;
        return WorkspaceDocumentChangeResult::SaveFailed;
    }
    return WorkspaceDocumentChangeResult::Removed;
}

bool WorkspaceManager::workspace_exists(const std::wstring& workspace_name) const {
    std::wstring normalized_name = normalize_workspace_name(workspace_name);
    if (normalized_name.empty()) {
        return false;
    }
    return find_workspace(normalized_name) != workspace_entries.end();
}

bool WorkspaceManager::workspace_contains_document(const std::wstring& workspace_name, const std::wstring& path) const {
    std::wstring normalized_name = normalize_workspace_name(workspace_name);
    std::wstring normalized_path = normalize_workspace_path(path);
    if (normalized_name.empty() || normalized_path.empty()) {
        return false;
    }

    auto workspace_it = find_workspace(normalized_name);
    if (workspace_it == workspace_entries.end()) {
        return false;
    }

    return std::any_of(workspace_it->documents.begin(), workspace_it->documents.end(), [&](const WorkspaceDocumentEntry& entry) {
        return entry.path == normalized_path;
        });
}

std::vector<std::wstring> WorkspaceManager::workspaces_containing_document(const std::wstring& path) const {
    std::vector<std::wstring> result;
    std::wstring normalized_path = normalize_workspace_path(path);
    if (normalized_path.empty()) {
        return result;
    }

    for (const WorkspaceEntry& workspace : workspace_entries) {
        if (std::any_of(workspace.documents.begin(), workspace.documents.end(), [&](const WorkspaceDocumentEntry& entry) {
            return entry.path == normalized_path;
            })) {
            result.push_back(workspace.name);
        }
    }
    return result;
}

WorkspacePathValidationResult WorkspaceManager::validate_workspace_paths(const std::wstring& workspace_name) const {
    WorkspacePathValidationResult result;
    std::wstring normalized_name = normalize_workspace_name(workspace_name);
    if (normalized_name.empty()) {
        return result;
    }

    auto workspace_it = find_workspace(normalized_name);
    if (workspace_it == workspace_entries.end()) {
        return result;
    }

    result.documents = static_cast<int>(workspace_it->documents.size());
    for (const WorkspaceDocumentEntry& document : workspace_it->documents) {
        QFileInfo file_info(QString::fromStdWString(document.path));
        if (!file_info.exists() || !file_info.isFile()) {
            result.missing++;
            result.missing_paths.push_back(document.path);
        }
    }
    return result;
}

std::optional<WorkspaceEntry> WorkspaceManager::get_workspace(const std::wstring& workspace_name) const {
    std::wstring normalized_name = normalize_workspace_name(workspace_name);
    if (normalized_name.empty()) {
        return {};
    }

    auto workspace_it = find_workspace(normalized_name);
    if (workspace_it == workspace_entries.end()) {
        return {};
    }
    return *workspace_it;
}

const std::vector<WorkspaceEntry>& WorkspaceManager::workspaces() const {
    return workspace_entries;
}

const std::vector<WorkspaceEntry>& WorkspaceManager::list_workspaces() const {
    return workspace_entries;
}

const std::wstring& WorkspaceManager::last_error() const {
    return last_error_message;
}

const std::wstring& WorkspaceManager::storage_path() const {
    return workspace_file_path;
}

void WorkspaceManager::set_active_workspace(const std::wstring& workspace_name) {
    std::wstring normalized_name = normalize_workspace_name(workspace_name);
    active_workspace_name = normalized_name.empty() ? std::optional<std::wstring>{} : std::optional<std::wstring>{ normalized_name };
    save();
}

void WorkspaceManager::clear_active_workspace() {
    active_workspace_name = {};
    save();
}

std::optional<std::wstring> WorkspaceManager::active_workspace() const {
    return active_workspace_name;
}

std::optional<std::wstring> WorkspaceManager::get_active_workspace() const {
    return active_workspace_name;
}

std::wstring WorkspaceManager::normalize_workspace_name(const std::wstring& workspace_name) {
    return QString::fromStdWString(workspace_name).trimmed().toStdWString();
}

std::wstring WorkspaceManager::normalize_workspace_path(const std::wstring& path) {
    return LibraryManager::normalize_library_path(path);
}
