#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <optional>
#include <variant>
#include <map>
#include <functional>
#include <qwidget.h>
#include <qabstractitemmodel.h>

#include "path.h"
#include "coordinates.h"

//#include <main_widget.h>



enum ConfigType {
    Int,
    Float,
    Color3,
    Color4,
    Bool,
    String,
    FilePath,
    FolderPath,
    IVec2,
    FVec2,
    EnableRectangle,
    Range,
    Macro
};

struct UIRange {
    float top;
    float bottom;
};

struct UIRect {
    bool enabled;
    float left;
    float right;
    float top;
    float bottom;


    bool contains(NormalizedWindowPos window_pos);
    QRect to_window(int window_width, int window_height);
};

struct FloatExtras {
    float min_val;
    float max_val;
};

struct IntExtras {
    int min_val;
    int max_val;
};

struct EmptyExtras {
};

struct AdditionalKeymapData {
    std::wstring file_name;
    int line_number;
    std::wstring keymap_string;
};
//union ConfigExtras {
//	struct Rest {
//
//	} rest;
//};

struct Config {

    std::wstring name;
    ConfigType config_type;
    void* value = nullptr;
    std::function<void(void*, std::wstringstream&)> serialize = nullptr;
    std::function<void*(std::wstringstream&, void* res, bool* changed)> deserialize = nullptr;
    std::function<bool(const std::wstring& value)> validator = nullptr;
    std::variant<FloatExtras, IntExtras, EmptyExtras> extras = EmptyExtras{};
    std::wstring default_value_string;
    bool is_auto = false;

    //    QWidget* (*configurator_ui)(MainWidget* main_widget, void* location);

    void* get_value();
    void save_value_into_default();
    void load_default();
    std::wstring get_type_string() const;
    std::wstring get_current_string();
    bool has_changed_from_default();
    bool is_empty_string();

};

class ConfigManager {

    std::vector<Config> configs;


    std::vector<Path> user_config_paths;

public:
    Config* get_mut_config_with_name(std::wstring config_name);

    ConfigManager(const Path& default_path, const Path& auto_path, const std::vector<Path>& user_paths);
    void serialize(const Path& path);
    void serialize_auto_configs(std::wofstream& stream);
    void restore_default();
    void clear_file(const Path& path);
    void persist_config();
    void deserialize(std::vector<std::string>* changed_config_names, const Path& default_file_path, const Path& auto_path, const std::vector<Path>& user_file_paths);
    void deserialize_file(std::vector<std::string>* changed_config_names, const Path& file_path, bool warn_if_not_exists = false, bool is_auto=false);
    template<typename T>
    const T* get_config(std::wstring name) {

        void* raw_pointer = get_mut_config_with_name(name)->get_value();

        // todo: Provide a default value for all configs, so that all the nullchecks here and in the
        // places where `get_config` is called can be removed.
        if (raw_pointer == nullptr) return nullptr;
        return (T*)raw_pointer;
    }
    std::optional<Path> get_or_create_user_config_file();
    std::vector<Path> get_all_user_config_files();
    std::vector<Config> get_configs();
    std::vector<Config>* get_configs_ptr();
    bool deserialize_config(std::string config_name, std::wstring config_value);
    void restore_defaults_in_memory();
    std::vector<std::wstring> get_auto_config_names();
};

class ConfigModel : public QAbstractTableModel {

private:
    std::vector<Config>* configs;

public:
    explicit ConfigModel(std::vector<Config>* configs, QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
};
