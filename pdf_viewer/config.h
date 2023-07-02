#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <optional>
#include <variant>
#include <map>
#include "path.h"
#include <qwidget.h>
#include <qabstractitemmodel.h>

//#include <main_widget.h>



enum ConfigType {
    Int,
    Float,
    Color3,
    Color4,
    Bool,
    String,
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

//union ConfigExtras {
//	struct Rest {
//
//	} rest;
//};

struct Config {

    std::wstring name;
    ConfigType config_type;
    void* value = nullptr;
    void (*serialize) (void*, std::wstringstream&) = nullptr;
    void* (*deserialize) (std::wstringstream&, void* res, bool* changed) = nullptr;
    bool (*validator) (const std::wstring& value) = nullptr;
    std::variant<FloatExtras, IntExtras, EmptyExtras> extras = EmptyExtras{};
    std::wstring default_value_string;

    //    QWidget* (*configurator_ui)(MainWidget* main_widget, void* location);

    void* get_value();
    void save_value_into_default();
    void load_default();

};

class ConfigManager {

    std::vector<Config> configs;


    std::vector<Path> user_config_paths;

public:
    Config* get_mut_config_with_name(std::wstring config_name);

    ConfigManager(const Path& default_path, const Path& auto_path, const std::vector<Path>& user_paths);
    void serialize(const Path& path);
    void restore_default();
    void clear_file(const Path& path);
    void persist_config();
    void deserialize(std::vector<std::string>* changed_config_names, const Path& default_file_path, const Path& auto_path, const std::vector<Path>& user_file_paths);
    void deserialize_file(std::vector<std::string>* changed_config_names, const Path& file_path, bool warn_if_not_exists = false);
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
