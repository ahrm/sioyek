#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include <optional>
#include <unordered_map>

#include <QListWidget>
#include <QScroller>
#include <qsizepolicy.h>
#include <qapplication.h>
#include <qqmlengine.h>
#include <qpushbutton.h>
#include <qopenglwidget.h>
#include <qopenglextrafunctions.h>
#include <qopenglfunctions.h>
#include <qopengl.h>
#include <qwindow.h>
#include <qkeyevent.h>
#include <qinputmethod.h>
#include <qlineedit.h>
#include <qtreeview.h>
#include <qsortfilterproxymodel.h>
#include <qabstractitemmodel.h>
#include <qopenglshaderprogram.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qstackedwidget.h>
#include <qboxlayout.h>
#include <qlistview.h>
#include <qtableview.h>
#include <qstringlistmodel.h>
#include <qpalette.h>
#include <qstandarditemmodel.h>
#include <qfilesystemmodel.h>
#include <qheaderview.h>
#include <qcolordialog.h>
#include <qslider.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <QQuickWidget>
#include "touchui/TouchSlider.h"
#include "touchui/TouchCheckbox.h"
#include "touchui/TouchListView.h"
#include "touchui/TouchCopyOptions.h"
#include "touchui/TouchRectangleSelectUI.h"
#include "touchui/TouchRangeSelectUI.h"
#include "touchui/TouchPageSelector.h"
#include "touchui/TouchMainMenu.h"
#include "touchui/TouchTextEdit.h"
#include "touchui/TouchSearchButtons.h"
#include "touchui/TouchDeleteButton.h"
#include "touchui/TouchHighlightButtons.h"
#include "touchui/TouchAudioButtons.h"
#include "touchui/TouchDrawControls.h"
#include "touchui/TouchMacroEditor.h"
#include "touchui/TouchGenericButtons.h"

#include "mysortfilterproxymodel.h"
#include "rapidfuzz_amalgamated.hpp"

#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "fts_fuzzy_match.h"
#undef FTS_FUZZY_MATCH_IMPLEMENTATION


#include "utils.h"
#include "config.h"

extern "C" {
    #include <fzf/fzf.h>
}

class MainWidget;
extern std::wstring UI_FONT_FACE_NAME;
extern int FONT_SIZE;
const int max_select_size = 100;
extern bool SMALL_TOC;
extern bool MULTILINE_MENUS;
extern bool EMACS_MODE;
extern bool TOUCH_MODE;


class HierarchialSortFilterProxyModel : public QSortFilterProxyModel {
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;
    //public:
    //	mutable int count = 0;
};

class ConfigFileChangeListener {
    static std::vector<ConfigFileChangeListener*> registered_listeners;


public:
    ConfigFileChangeListener();
    ~ConfigFileChangeListener();
    virtual void on_config_file_changed(ConfigManager* new_config_manager) = 0;
    static void notify_config_file_changed(ConfigManager* new_config_manager);
};


class MyLineEdit: public QLineEdit {
    Q_OBJECT

public:
    MyLineEdit(QWidget* parent=nullptr);

    void keyPressEvent(QKeyEvent* event) override;
    int get_next_word_position();
    int get_prev_word_position();

signals:
    void next_suggestion();
    void prev_suggestion();
};

class BaseSelectorWidget : public QWidget {

protected:
    BaseSelectorWidget(QAbstractItemView* item_view, bool fuzzy, QStandardItemModel* item_model, QWidget* parent);

    void on_text_changed(const QString& text);



    virtual QAbstractItemView* get_view();

    QLineEdit* line_edit = nullptr;
    //QSortFilterProxyModel* proxy_model = nullptr;
    MySortFilterProxyModel* proxy_model = nullptr;
    bool is_fuzzy = false;
    int pressed_row = -1;
    QPoint pressed_pos;

    QAbstractItemView* abstract_item_view;

    virtual void on_select(const QModelIndex& value) = 0;
    virtual void on_delete(const QModelIndex& source_index, const QModelIndex& selected_index);
    virtual void on_edit(const QModelIndex& source_index, const QModelIndex& selected_index);

    virtual void on_return_no_select(const QString& text);

    // should return true when we want to manually handle text change events
    virtual bool on_text_change(const QString& text);

    virtual QString get_view_stylesheet_type_name() = 0;

public:


    void set_filter_column_index(int index);
    std::optional<QModelIndex> get_selected_index();
    virtual std::wstring get_selected_text();
    bool eventFilter(QObject* obj, QEvent* event) override;
    void simulate_move_down();
    void simulate_move_up();
    void simulate_select();
    void handle_delete();
    void handle_edit();
    QString get_selected_item();


#ifndef SIOYEK_QT6
    void keyReleaseEvent(QKeyEvent* event) override;
#endif

    virtual void on_config_file_changed();
    void resizeEvent(QResizeEvent* resize_event) override;

};

template<typename T>
class FilteredTreeSelect : public BaseSelectorWidget {
private:
    std::function<void(const std::vector<int>&)> on_done;

protected:

public:

    QString get_view_stylesheet_type_name() {
        return "QTreeView";
    }

    FilteredTreeSelect(bool fuzzy, QStandardItemModel* item_model,
        std::function<void(const std::vector<int>&)> on_done,
        QWidget* parent,
        std::vector<int> selected_index) : BaseSelectorWidget(new QTreeView(), fuzzy, item_model, parent),
        on_done(on_done)
    {
        auto index = QModelIndex();
        for (auto i : selected_index) {
            index = this->proxy_model->index(i, 0, index);
        }

        QTreeView* tree_view = dynamic_cast<QTreeView*>(this->get_view());
        if (SMALL_TOC) {
            tree_view->collapseAll();
            tree_view->expand(index);
        }

        tree_view->header()->setStretchLastSection(false);
        tree_view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        tree_view->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        tree_view->setCurrentIndex(index);

    }

    void on_select(const QModelIndex& index) {
        this->hide();
        this->parentWidget()->setFocus();
        auto source_index = this->proxy_model->mapToSource(index);
        std::vector<int> indices;
        while (source_index != QModelIndex()) {
            indices.push_back(source_index.row());
            source_index = source_index.parent();
        }
        on_done(indices);
    }
};

template<typename T>
class FilteredSelectTableWindowClass : public BaseSelectorWidget {
private:

    QStringListModel* string_list_model = nullptr;
    std::vector<T> values;
    std::vector<std::wstring> item_strings;
    std::function<void(T*)> on_done = nullptr;
    std::function<void(T*)> on_delete_function = nullptr;
    std::function<void(T*)> on_edit_function = nullptr;
    int n_cols = 1;

protected:

public:

    QString get_view_stylesheet_type_name() {
        return "QTableView";
    }

    FilteredSelectTableWindowClass(
        bool fuzzy,
        bool multiline,
        const std::vector<std::vector<std::wstring>>& table,
        std::vector<T> values,
        int selected_index,
        std::function<void(T*)> on_done,
        QWidget* parent,
        std::function<void(T*)> on_delete_function = nullptr) : BaseSelectorWidget(new QTableView(), fuzzy, nullptr, parent),
        values(values),
        on_done(on_done),
        on_delete_function(on_delete_function)
    {
        item_strings = table[0];
        QVector<QString> q_string_list;
        for (const auto& s : table[0]) {
            q_string_list.push_back(QString::fromStdWString(s));

        }


        QStandardItemModel* model = create_table_model(table);
        //model->setItem(selected_index, 0, new QStandardItem(QString::fromStdWString(std_string_list[selected_index])));
        //QStandardItemModel* model = new QStandardItemModel();

        //for (size_t i = 0; i < std_string_list.size(); i++) {
        //	QStandardItem* name_item = new QStandardItem(QString::fromStdWString(std_string_list[i]));
        //	QStandardItem* key_item = new QStandardItem(QString::fromStdWString(std_string_list_right[i]));
        //	key_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        //	model->appendRow(QList<QStandardItem*>() << name_item << key_item);
        //}

        this->proxy_model->setSourceModel(model);

        QTableView* table_view = dynamic_cast<QTableView*>(this->get_view());
        //        QScroller::grabGesture(table_view, QScroller::LeftMouseButtonGesture);
        //        table_view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        //        table_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);


        if (selected_index != -1) {
            table_view->selectionModel()->setCurrentIndex(model->index(selected_index, 0), QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
        }
        else {
            table_view->selectionModel()->setCurrentIndex(model->index(0, 0), QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
        }

        table_view->setSelectionMode(QAbstractItemView::SingleSelection);
        table_view->setSelectionBehavior(QAbstractItemView::SelectRows);
        table_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

        if (table[0].size() > 0) {
            n_cols = table.size();
            table_view->horizontalHeader()->setStretchLastSection(false);
            for (int i = 0; i < table.size() - 1; i++) {
                table_view->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
            }

            table_view->horizontalHeader()->setSectionResizeMode(table.size() - 1, QHeaderView::ResizeToContents);
        }

        table_view->horizontalHeader()->hide();
        table_view->verticalHeader()->hide();

        if (multiline) {
            table_view->setWordWrap(true);
            table_view->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        }

        if (selected_index != -1) {
            table_view->scrollTo(this->proxy_model->mapFromSource(table_view->currentIndex()), QAbstractItemView::EnsureVisible);
        }
    }

    void set_equal_columns() {
        QTableView* table_view = dynamic_cast<QTableView*>(get_view());
        for (int i = 0; i < n_cols; i++) {
            table_view->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
        }

    }

    void set_on_edit_function(std::function<void(T*)> edit_func) {
        on_edit_function = edit_func;
    }

    void set_value_second_item(T value, QString str) {

        for (int i = 0; i < values.size(); i++) {
            if (values[i] == value) {
                auto source_model = this->proxy_model->sourceModel();
                source_model->setData(source_model->index(i, 1), str);
                return;
            }
        }

    }

    virtual std::wstring get_selected_text() {
        auto index = this->get_selected_index();

        if (index) {

            auto source_index = this->proxy_model->mapToSource(index.value());
            return item_strings[source_index.row()];
        }

        return L"";
    }

    virtual void on_delete(const QModelIndex& source_index, const QModelIndex& selected_index) override {
        if (on_delete_function) {
            on_delete_function(&values[source_index.row()]);
            this->proxy_model->removeRow(selected_index.row());
            values.erase(values.begin() + source_index.row());
        }
    }

    virtual void on_edit(const QModelIndex& source_index, const QModelIndex& selected_index) override {
        if (on_edit_function) {
            on_edit_function(&values[source_index.row()]);
        }
    }

    void on_select(const QModelIndex& index) {
        this->hide();
        this->parentWidget()->setFocus();
        auto source_index = this->proxy_model->mapToSource(index);
        on_done(&values[source_index.row()]);
    }
};

template <typename T>
class TouchFilteredSelectWidget : public QWidget {
private:
    //    QStringListModel string_list_model;
    //    MySortFilterProxyModel proxy_model;
    TouchListView* list_view = nullptr;
    QWidget* parent_widget;
    std::vector<T> values;
    std::function<void(T*)> on_done;
    std::function<void(T*)> on_delete_function = nullptr;

public:
    //	void set_selected_index(int index) {
    //		list_view->set_selected_index(index);
    //	}
    void initialize() {

        QObject::connect(list_view, &TouchListView::itemSelected, [&](QString name, int index) {
            on_done(&values[index]);
            deleteLater();
            });

        QObject::connect(list_view, &TouchListView::itemDeleted, [&](QString name, int index) {
            on_delete_function(&values[index]);
            //deleteLater();
            });
    }
    TouchFilteredSelectWidget(bool is_fuzzy, std::vector<std::wstring> std_string_list,
        std::vector<T> values_,
        int selected_index,
        std::function<void(T*)> on_done_,
        std::function<void(T*)> on_delete,
        QWidget* parent) :
        QWidget(parent),
        values(values_),
        on_done(on_done_),
        on_delete_function(on_delete) {

        parent_widget = parent;
        QStringList string_list;
        for (auto s : std_string_list) {
            string_list.append(QString::fromStdWString(s));
        }
        //        string_list_model.setStringList(string_list);
        //        proxy_model.setSourceModel(string_list_model);

        list_view = new TouchListView(is_fuzzy, string_list, selected_index, this, true);
        initialize();
    }

    TouchFilteredSelectWidget(bool is_fuzzy, QAbstractItemModel* model,
        int selected_index,
        std::function<void(T*)> on_done_,
        QWidget* parent) :
        QWidget(parent),
        on_done(on_done_) {
        parent_widget = parent;
        list_view = new TouchListView(is_fuzzy, model, selected_index, this, false, false, true);

        QObject::connect(list_view, &TouchListView::itemSelected, [&](QString name, int index) {
            on_done(&values[index]);
            deleteLater();
            });
    }

    TouchFilteredSelectWidget(bool is_fuzzy, QAbstractItemModel* model,
        std::vector<T> values_,
        int selected_index,
        std::function<void(T*)> on_done_,
        std::function<void(T*)> on_delete,
        QWidget* parent) :
        QWidget(parent),
        values(values_),
        on_done(on_done_),
        on_delete_function(on_delete) {

        parent_widget = parent;
        //        string_list_model.setStringList(string_list);
        //        proxy_model.setSourceModel(string_list_model);

        list_view = new TouchListView(is_fuzzy, model, selected_index, this, true);
        initialize();
    }

    void resizeEvent(QResizeEvent* resize_event) override {
        QWidget::resizeEvent(resize_event);
        int parent_width = parentWidget()->size().width();
        int parent_height = parentWidget()->size().height();
        //        setFixedSize(parent_width * 0.9f, parent_height);
        list_view->resize(parent_width * 0.9f, parent_height);
        move(parent_width * 0.05f, 0);
        resize(parent_width * 0.9f, parent_height);
    }

    void set_filter_column_index(int index) {
        list_view->proxy_model->setFilterKeyColumn(index);
    }

    void set_value_second_item(T value, QString str) {

        auto source_model = list_view->proxy_model->sourceModel();

        if (source_model->columnCount() < 2) return;

        for (int i = 0; i < values.size(); i++) {
            if (values[i] == value) {
                source_model->setData(source_model->index(i, 1), str);
                list_view->update_model();
                return;
            }
        }

    }
};


template<typename T>
class FilteredSelectWindowClass : public BaseSelectorWidget {
private:

    QStringListModel* string_list_model = nullptr;
    std::vector<T> values;
    std::function<void(T*)> on_done = nullptr;
    std::function<void(T*)> on_delete_function = nullptr;

protected:

public:

    QString get_view_stylesheet_type_name() {
        return "QListView";
    }


    FilteredSelectWindowClass(bool fuzzy, std::vector<std::wstring> std_string_list,
        std::vector<T> values,
        std::function<void(T*)> on_done,
        QWidget* parent,
        std::function<void(T*)> on_delete_function = nullptr, int selected_index = -1) : BaseSelectorWidget(new QListView(), fuzzy, nullptr, parent),
        values(values),
        on_done(on_done),
        on_delete_function(on_delete_function)
    {
        QVector<QString> q_string_list;
        for (const auto& s : std_string_list) {
            q_string_list.push_back(QString::fromStdWString(s));

        }
        QStringList string_list = QStringList::fromVector(q_string_list);


        string_list_model = new QStringListModel(string_list);
        this->proxy_model->setSourceModel(string_list_model);

        if (selected_index != -1) {
            dynamic_cast<QListView*>(this->get_view())->selectionModel()->setCurrentIndex(string_list_model->index(selected_index), QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
        }
        else {
            dynamic_cast<QListView*>(this->get_view())->selectionModel()->setCurrentIndex(string_list_model->index(0, 0), QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
        }

    }

    virtual void on_delete(const QModelIndex& source_index, const QModelIndex& selected_index) override {
        if (on_delete_function) {
            on_delete_function(&values[source_index.row()]);
            this->proxy_model->removeRow(selected_index.row());
            values.erase(values.begin() + source_index.row());
        }
    }

    void on_select(const QModelIndex& index) {
        this->hide();
        this->parentWidget()->setFocus();
        auto source_index = this->proxy_model->mapToSource(index);
        on_done(&values[source_index.row()]);
    }
};

class TouchCommandSelector : public QWidget {
public:
    TouchCommandSelector(bool is_fuzzy, const QStringList& commands, MainWidget* mw);
    void resizeEvent(QResizeEvent* resize_event) override;
    //    void keyReleaseEvent(QKeyEvent* key_event) override;

private:
    MainWidget* main_widget;
    TouchListView* list_view;

};

class CommandSelector : public BaseSelectorWidget {
private:
    QStringList string_elements;
    MainWidget* main_widget;
    QStandardItemModel* standard_item_model = nullptr;
    std::unordered_map<std::string, std::vector<std::string>> key_map;
    std::function<void(std::string, std::string)>* on_done = nullptr;
    fzf_slab_t* slab;

    QList<QStandardItem*> get_item(std::string command_name);
    QStandardItemModel* get_standard_item_model(std::vector<std::string> command_names);
    QStandardItemModel* get_standard_item_model(QStringList command_names);

protected:
public:

    QString get_view_stylesheet_type_name();

    void on_select(const QModelIndex& index);

    CommandSelector(bool is_fuzzy, std::function<void(std::string, std::string)>* on_done,
        MainWidget* parent,
        QStringList elements,
        std::unordered_map<std::string,
        std::vector<std::string>> key_map);
    ~CommandSelector();

    virtual bool on_text_change(const QString& text);

};

//class FileSelector : public BaseSelectorWidget<std::wstring, QListView, QSortFilterProxyModel> {
class FileSelector : public BaseSelectorWidget {
private:

    QStringListModel* list_model = nullptr;
    std::function<void(std::wstring)> on_done = nullptr;
    QString last_root = "";

protected:

public:

    QString get_view_stylesheet_type_name() {
        return "QListView";
    }

    FileSelector(bool is_fuzzy, std::function<void(std::wstring)> on_done, QWidget* parent, QString last_path) :
        BaseSelectorWidget(new QListView(), is_fuzzy, nullptr, parent),
        on_done(on_done)
    {


        QString root_path;
        QString file_name;

        if (last_path.size() > 0) {
            split_root_file(last_path, root_path, file_name);
            root_path += QDir::separator();
        }

        QStringList dir_contents = get_dir_contents(root_path, "");
        int current_index = -1;
        for (int i = 0; i < dir_contents.size(); i++) {
            if (dir_contents.at(i) == file_name) {
                current_index = i;
                break;
            }
        }

        list_model = new QStringListModel(dir_contents);
        last_root = root_path;
        line_edit->setText(last_root);


        dynamic_cast<QListView*>(get_view())->setModel(list_model);

        if (current_index != -1) {
            dynamic_cast<QListView*>(get_view())->setCurrentIndex(list_model->index(current_index));
        }
        else {
            dynamic_cast<QListView*>(get_view())->setCurrentIndex(list_model->index(0, 0));
        }
        //dynamic_cast<QListView*>(get_view())->setCurrentIndex();
    }

    virtual bool on_text_change(const QString& text) {
        QString root_path;
        QString partial_name;
        split_root_file(text, root_path, partial_name);

        last_root = root_path;
        if (last_root.size() > 0) {
            if (last_root.at(last_root.size() - 1) == QDir::separator()) {
                last_root.chop(1);
            }
        }

        QStringList match_list = get_dir_contents(root_path, partial_name);
        QStringListModel* new_list_model = new QStringListModel(match_list);
        dynamic_cast<QListView*>(get_view())->setModel(new_list_model);
        delete list_model;
        list_model = new_list_model;
        dynamic_cast<QListView*>(get_view())->setCurrentIndex(list_model->index(0, 0));
        return true;
    }

    QStringList get_dir_contents(QString root, QString prefix) {

        root = expand_home_dir(root);
        QDir directory(root);
        QStringList res = directory.entryList({ prefix + "*" });
        if (res.size() == 0) {
            std::string encoded_prefix = utf8_encode(prefix.toStdWString());
            QStringList all_directory_files = directory.entryList();
            std::vector<std::pair<QString, int>> file_scores;

            for (auto file : all_directory_files) {
                std::string encoded_file = utf8_encode(file.toStdWString());
                int score = 0;
                if (is_fuzzy) {
                    score = static_cast<int>(rapidfuzz::fuzz::partial_ratio(encoded_prefix, encoded_file));
                }
                else {
                    fts::fuzzy_match(encoded_prefix.c_str(), encoded_file.c_str(), score);
                }
                file_scores.push_back(std::make_pair(file, score));
            }
            std::sort(file_scores.begin(), file_scores.end(), [](std::pair<QString, int> lhs, std::pair<QString, int> rhs) {
                return lhs.second > rhs.second;
                });
            for (auto [file, score] : file_scores) {
                if (score > 0) {
                    res.push_back(file);
                }
            }
        }
        return res;
    }

    void on_select(const QModelIndex& index) {
        QString name = list_model->data(index).toString();
        QChar sep = QDir::separator();
        QString full_path = expand_home_dir((last_root.size() > 0) ? (last_root + sep + name) : name);

        if (QFileInfo(full_path).isFile()) {
            on_done(full_path.toStdWString());
            hide();
            parentWidget()->setFocus();
        }
        else {
            line_edit->setText(full_path + sep);
        }
    }
};

class AndroidSelector : public QWidget {
public:

    AndroidSelector(QWidget* parent);
    void resizeEvent(QResizeEvent* resize_event) override;
    //    bool event(QEvent *event) override;
private:
    //    QVBoxLayout* layout;
    //    QPushButton* fullscreen_button;
    //    QPushButton* select_text_button;
    //    QPushButton* open_document_button;
    //    QPushButton* open_prev_document_button;
    //    QPushButton* command_button;
    //    QPushButton* visual_mode_button;
    //    QPushButton* search_button;
    //    QPushButton* set_background_color;
    //    QPushButton* set_dark_mode_contrast;
    //    QPushButton* set_ruler_mode;
    //    QPushButton* restore_default_config_button;
    //    QPushButton* toggle_dark_mode_button;
    //    QPushButton* ruler_mode_bounds_config_button;
    //    QPushButton* goto_page_button;
    //    QPushButton* set_rect_config_button;
    //    QPushButton* test_rectangle_select_ui;
    TouchMainMenu* main_menu;


    MainWidget* main_widget;

};

//class TextSelectionButtons : public QWidget{
//public:
//    TextSelectionButtons(MainWidget* parent);
//    void resizeEvent(QResizeEvent* resize_event) override;
//private:

//    MainWidget* main_widget;
//    QHBoxLayout* layout;
//    QPushButton* copy_button;
//    QPushButton* search_in_scholar_button;
//    QPushButton* search_in_google_button;
//    QPushButton* highlight_button;

//};


class DrawControlsUI : public QWidget {
public:
    DrawControlsUI(MainWidget* parent);
    void resizeEvent(QResizeEvent* resize_event) override;
    TouchDrawControls* controls_ui;
private:
    MainWidget* main_widget;

};

class TouchTextSelectionButtons : public QWidget {
public:
    TouchTextSelectionButtons(MainWidget* parent);
    void resizeEvent(QResizeEvent* resize_event) override;
private:
    MainWidget* main_widget;

    TouchCopyOptions* buttons_ui;
};

class HighlightButtons : public QWidget {

public:
    HighlightButtons(MainWidget* parent);
    void resizeEvent(QResizeEvent* resize_event) override;
    TouchHighlightButtons* highlight_buttons;
private:

    MainWidget* main_widget;
    //TouchDeleteButton* delete_button;
    //QQuickWidget* buttons_widget;
    //QHBoxLayout* layout;
    //QPushButton* delete_highlight_button;
};

class SearchButtons : public QWidget {

public:
    SearchButtons(MainWidget* parent);
    void resizeEvent(QResizeEvent* resize_event) override;
private:

    MainWidget* main_widget;
    TouchSearchButtons* buttons_widget;
    //QHBoxLayout* layout;
    //QPushButton* prev_match_button;
    //QPushButton* next_match_button;
    //QPushButton* goto_initial_location_button;
};

class ConfigUI : public QWidget {
    //class ConfigUI : public QQuickWidget{
public:
    ConfigUI(std::string name, MainWidget* parent);
    void resizeEvent(QResizeEvent* resize_event) override;
    void set_should_persist(bool val);
    void on_change();

protected:
    MainWidget* main_widget;
    std::string config_name;
    bool should_persist = true;
};

class Color3ConfigUI : public ConfigUI {
public:
    Color3ConfigUI(std::string name, MainWidget* parent, float* config_location_);
    void resizeEvent(QResizeEvent* resize_event) override;

private:
    float* color_location;
    QColorDialog* color_picker;
    //    QQuickWidget* color_picker;

};

class Color4ConfigUI : public ConfigUI {
public:
    Color4ConfigUI(std::string name, MainWidget* parent, float* config_location_);

private:
    float* color_location;
    QColorDialog* color_picker;

};

class BoolConfigUI : public ConfigUI {
public:
    BoolConfigUI(std::string name, MainWidget* parent, bool* config_location, QString name_);
    void resizeEvent(QResizeEvent* resize_event) override;
private:

    bool* bool_location;
    TouchCheckbox* checkbox;
    //   	TouchCh
    //    QHBoxLayout* layout;
    //    QCheckBox* checkbox;
    //    QLabel* label;

};


//class TextConfigUI : public ConfigUI{
//public:
//	TextConfigUI(MainWidget* parent, std::wstring* config_location);
//    void resizeEvent(QResizeEvent* resize_event) override;
//private:
//    std::wstring* float_location;
//    TouchTextEdit* text_edit = nullptr;
//};

class FloatConfigUI : public ConfigUI {
public:
    FloatConfigUI(std::string name, MainWidget* parent, float* config_location, float min_value, float max_value);
    void resizeEvent(QResizeEvent* resize_event) override;
private:
    float* float_location;
    float min_value;
    float max_value;
    TouchSlider* slider = nullptr;
};

class MacroConfigUI : public ConfigUI {
public:
    MacroConfigUI(std::string name, MainWidget* parent, std::wstring* config_location, std::wstring initial_macro);
    void resizeEvent(QResizeEvent* resize_event) override;
private:
    TouchMacroEditor* macro_editor = nullptr;
};

class IntConfigUI : public ConfigUI {
public:
    IntConfigUI(std::string name, MainWidget* parent, int* config_location, int min_value, int max_value);
    void resizeEvent(QResizeEvent* resize_event) override;
private:
    int* int_location;
    int min_value;
    int max_value;
    TouchSlider* slider = nullptr;
};

class PageSelectorUI : public ConfigUI {
public:
    PageSelectorUI(MainWidget* parent, int current, int num_pages);
    void resizeEvent(QResizeEvent* resize_event) override;
private:
    TouchPageSelector* page_selector = nullptr;
};

class AudioUI : public ConfigUI {
public:
    AudioUI(MainWidget* parent);
    void resizeEvent(QResizeEvent* resize_event) override;
private:
    TouchAudioButtons* buttons = nullptr;
};

class RectangleConfigUI : public ConfigUI {
public:
    RectangleConfigUI(std::string name, MainWidget* parent, UIRect* config_location);

    void resizeEvent(QResizeEvent* resize_event) override;
private:
    UIRect* rect_location;

    TouchRectangleSelectUI* rectangle_select_ui = nullptr;
};

class RangeConfigUI : public ConfigUI {
public:
    RangeConfigUI(std::string name, MainWidget* parent, float* top_location, float* bottom_location);

    void resizeEvent(QResizeEvent* resize_event) override;
private:
    float* top_location;
    float* bottom_location;

    TouchRangeSelectUI* range_select_ui = nullptr;
};


std::wstring select_document_file_name();
std::wstring select_json_file_name();
std::wstring select_any_file_name();
std::wstring select_command_file_name(std::string command_name);
std::wstring select_new_json_file_name();
std::wstring select_new_pdf_file_name();
std::wstring select_command_folder_name();

//QWidget* color3_configurator_ui(MainWidget* main_widget, void* location);
//QWidget* color4_configurator_ui(MainWidget* main_widget, void* location);

//template<float min_value, float max_value>
//QWidget* float_configurator_ui(MainWidget* main_widget, void* location){
//    return new FloatConfigUI(main_widget, (float*)location, min_value, max_value);
//}

//template<QString name>
//QWidget* bool_configurator_ui(MainWidget* main_widget, void* location){
//    return new BoolConfigUI(main_widget, (float*)location, name);
//}
