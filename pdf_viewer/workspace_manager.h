#pragma once

#include <optional>
#include <string>
#include <vector>

struct WorkspaceDocumentEntry {
    std::wstring path;
    std::wstring display_name;
    int page = -1;
    float zoom_level = 0.0f;
    float offset_x = 0.0f;
    float offset_y = 0.0f;
    std::optional<int> window_index;
    std::string last_saved;
};

struct WorkspaceEntry {
    std::wstring name;
    std::string created_at;
    std::string updated_at;
    std::vector<WorkspaceDocumentEntry> documents;
};

enum class WorkspaceSaveResult {
    Saved,
    InvalidName,
    EmptyWorkspace,
    SaveFailed,
};

enum class WorkspaceDeleteResult {
    Deleted,
    NotFound,
    InvalidName,
    SaveFailed,
};

enum class WorkspaceRenameResult {
    Renamed,
    NotFound,
    AlreadyExists,
    InvalidName,
    SaveFailed,
};

enum class WorkspaceDocumentChangeResult {
    Added,
    Updated,
    Removed,
    WorkspaceNotFound,
    DocumentNotFound,
    InvalidWorkspaceName,
    InvalidPath,
    SaveFailed,
};

struct WorkspacePathValidationResult {
    int documents = 0;
    int missing = 0;
    std::vector<std::wstring> missing_paths;
};

class WorkspaceManager {
private:
    std::wstring workspace_file_path;
    std::vector<WorkspaceEntry> workspace_entries;
    std::wstring last_error_message;
    std::optional<std::wstring> active_workspace_name;

    std::vector<WorkspaceEntry>::iterator find_workspace(const std::wstring& workspace_name);
    std::vector<WorkspaceEntry>::const_iterator find_workspace(const std::wstring& workspace_name) const;
    void set_last_error(const std::wstring& message);

public:
    explicit WorkspaceManager(const std::wstring& workspace_file_path);

    bool load();
    bool save();

    WorkspaceSaveResult save_workspace(const WorkspaceEntry& workspace);
    WorkspaceSaveResult save_workspace(const std::wstring& workspace_name, const std::vector<WorkspaceDocumentEntry>& documents);
    WorkspaceSaveResult update_workspace_from_current_session(const std::wstring& workspace_name, const std::vector<WorkspaceDocumentEntry>& documents);
    WorkspaceDeleteResult delete_workspace(const std::wstring& workspace_name);
    WorkspaceRenameResult rename_workspace(const std::wstring& old_name, const std::wstring& new_name);
    WorkspaceDocumentChangeResult add_or_update_document(const std::wstring& workspace_name, const WorkspaceDocumentEntry& document);
    WorkspaceDocumentChangeResult add_current_document_to_workspace(const std::wstring& workspace_name, const WorkspaceDocumentEntry& document);
    WorkspaceDocumentChangeResult remove_document_from_workspace(const std::wstring& workspace_name, const std::wstring& path);

    bool workspace_exists(const std::wstring& workspace_name) const;
    bool workspace_contains_document(const std::wstring& workspace_name, const std::wstring& path) const;
    std::vector<std::wstring> workspaces_containing_document(const std::wstring& path) const;
    WorkspacePathValidationResult validate_workspace_paths(const std::wstring& workspace_name) const;
    std::optional<WorkspaceEntry> get_workspace(const std::wstring& workspace_name) const;
    const std::vector<WorkspaceEntry>& list_workspaces() const;
    const std::vector<WorkspaceEntry>& workspaces() const;
    const std::wstring& last_error() const;
    const std::wstring& storage_path() const;

    void set_active_workspace(const std::wstring& workspace_name);
    void clear_active_workspace();
    std::optional<std::wstring> active_workspace() const;
    std::optional<std::wstring> get_active_workspace() const;

    static std::wstring normalize_workspace_name(const std::wstring& workspace_name);
    static std::wstring normalize_workspace_path(const std::wstring& path);
};
