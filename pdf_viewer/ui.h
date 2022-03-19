#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include <optional>
#include <unordered_map>

#include <qsizepolicy.h>
#include <qapplication.h>
#include <qpushbutton.h>
#include <qopenglwidget.h>
#include <qopenglextrafunctions.h>
#include <qopenglfunctions.h>
#include <qopengl.h>
#include <qwindow.h>
#include <qkeyevent.h>
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

#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "fts_fuzzy_match.h"
#undef FTS_FUZZY_MATCH_IMPLEMENTATION

#include "utils.h"
#include "config.h"

extern std::wstring UI_FONT_FACE_NAME;
extern int FONT_SIZE;
const int max_select_size = 100;
extern bool SMALL_TOC;

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

template <typename T, typename ViewType, typename ProxyModelType>
class BaseSelectorWidget : public QWidget {

protected:
	BaseSelectorWidget(QStandardItemModel* item_model, QWidget* parent) : QWidget(parent) {

		proxy_model = new ProxyModelType;
		proxy_model->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

		if (item_model) {
			proxy_model->setSourceModel(item_model);
		}

		resize(300, 800);
		QVBoxLayout* layout = new QVBoxLayout;
		setLayout(layout);

		line_edit = new QLineEdit;
		abstract_item_view = new ViewType;
		abstract_item_view->setModel(proxy_model);
		abstract_item_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

		QTreeView* tree_view = dynamic_cast<QTreeView*>(abstract_item_view);
		if (tree_view) {
			tree_view->expandAll();
			tree_view->setHeaderHidden(true);
			tree_view->resizeColumnToContents(0);
			tree_view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
		}
		QSortFilterProxyModel* sort_filter_proxy_model = dynamic_cast<QSortFilterProxyModel*>(proxy_model);
		if (proxy_model) {
			proxy_model->setRecursiveFilteringEnabled(true);
		}

		layout->addWidget(line_edit);
		layout->addWidget(abstract_item_view);

		line_edit->installEventFilter(this);
		line_edit->setFocus();

		QObject::connect(abstract_item_view, &QAbstractItemView::activated, [&](const QModelIndex& index) {
			on_select(index);
			});

		QObject::connect(line_edit, &QLineEdit::textChanged, [&](const QString& text) {
			if (!on_text_change(text)) {
				// generic text change handling when we don't explicitly handle text change events
				proxy_model->setFilterFixedString(text);
				QTreeView* t_view = dynamic_cast<QTreeView*>(get_view());
				if (t_view) {
					t_view->expandAll();
				}
			}
			});
	}


	QAbstractItemView* get_view() {
		return abstract_item_view;
	}

	QLineEdit* line_edit = nullptr;
	QSortFilterProxyModel* proxy_model = nullptr;
	ViewType* abstract_item_view;

	virtual void on_select(const QModelIndex& value) = 0;
	virtual void on_delete(const QModelIndex& source_index, const QModelIndex& selected_index) {}
	virtual void on_return_no_select(const QString& text) {}

	// should return true when we want to manually handle text change events
	virtual bool on_text_change(const QString& text) {
		return false;
	}

	virtual QString get_view_stylesheet_type_name() = 0;

public:


	std::optional<QModelIndex> get_selected_index() {
		QModelIndexList selected_index_list = get_view()->selectionModel()->selectedIndexes();

		if (selected_index_list.size() > 0) {
			QModelIndex selected_index = selected_index_list.at(0);
			return selected_index;
		}
		return {};
	}

	bool eventFilter(QObject* obj, QEvent* event) override {
		if (obj == line_edit) {
			if (event->type() == QEvent::KeyPress) {
				QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
				bool is_control_pressed = key_event->modifiers().testFlag(Qt::ControlModifier) || key_event->modifiers().testFlag(Qt::MetaModifier);

				if (key_event->key() == Qt::Key_Down ||
					key_event->key() == Qt::Key_Up ||
					key_event->key() == Qt::Key_Left ||
					key_event->key() == Qt::Key_Right
					) {
					QKeyEvent* newEvent = new QKeyEvent(*key_event);
					QCoreApplication::postEvent(get_view(), newEvent);
					//QCoreApplication::postEvent(tree_view, key_event);
					return true;
				}
				if (key_event->key() == Qt::Key_Tab) {
					QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Down, key_event->modifiers());
					QCoreApplication::postEvent(get_view(), new_key_event);
					return true;
				}
				if ((key_event->key() == Qt::Key_N) && is_control_pressed) {
					QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Down, key_event->modifiers());
					QCoreApplication::postEvent(get_view(), new_key_event);
					return true;
				}
				if ((key_event->key() == Qt::Key_P) && is_control_pressed) {
					QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Up, key_event->modifiers());
					QCoreApplication::postEvent(get_view(), new_key_event);
					return true;
				}
				if (key_event->key() == Qt::Key_Backtab) {
					QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Up, key_event->modifiers());
					QCoreApplication::postEvent(get_view(), new_key_event);
					return true;
				}
				if (key_event->key() == Qt::Key_Return || key_event->key() == Qt::Key_Enter) {
					std::optional<QModelIndex> selected_index = get_selected_index();
					if (selected_index) {
						on_select(selected_index.value());
					}
					else {
						on_return_no_select(line_edit->text());
					}
				}

			}
		}
		return false;
	}

	void keyReleaseEvent(QKeyEvent* event) override {
		if (event->key() == Qt::Key_Delete) {
			QModelIndexList selected_index_list = get_view()->selectionModel()->selectedIndexes();
			if (selected_index_list.size() > 0) {
				QModelIndex selected_index = selected_index_list.at(0);
				if (proxy_model->hasIndex(selected_index.row(), selected_index.column())) {
					QModelIndex source_index = proxy_model->mapToSource(selected_index);
					on_delete(source_index, selected_index);
				}
			}
		}
		QWidget::keyReleaseEvent(event);
	}

	virtual void on_config_file_changed() {
		QString font_size_stylesheet = "";
		if (FONT_SIZE > 0) {
			font_size_stylesheet = QString("font-size: %1px").arg(FONT_SIZE);
		}

		setStyleSheet("background-color: black; color: white; border: 0;" + font_size_stylesheet);
		get_view()->setStyleSheet(get_view_stylesheet_type_name() + "::item::selected{background-color: white; color: black;}");
	}

	void resizeEvent(QResizeEvent* resize_event) override {
		QWidget::resizeEvent(resize_event);
		int parent_width = parentWidget()->width();
		int parent_height = parentWidget()->height();
		setFixedSize(parent_width * 0.9f, parent_height);
		move(parent_width * 0.05f, 0);
		on_config_file_changed();
	}

};

template<typename T>
class FilteredTreeSelect : public BaseSelectorWidget<T, QTreeView, QSortFilterProxyModel> {
private:
	std::function<void(const std::vector<int>&)> on_done;

protected:

public:

	QString get_view_stylesheet_type_name() {
		return "QTreeView";
	}

	FilteredTreeSelect(QStandardItemModel* item_model,
		std::function<void(const std::vector<int>&)> on_done,
		QWidget* parent,
		std::vector<int> selected_index) : BaseSelectorWidget<T, QTreeView, QSortFilterProxyModel>(item_model, parent),
		on_done(on_done)
	{
		auto index = QModelIndex();
		for (auto i : selected_index) {
			index = this->proxy_model->index(i, 0, index);
		}

        QTreeView* tree_view = dynamic_cast<QTreeView*>(this->get_view());
        if (SMALL_TOC){
            tree_view->collapseAll();
            tree_view->expand(index);
        }

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
class FilteredSelectWindowClass : public BaseSelectorWidget<T, QListView, QSortFilterProxyModel> {
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

	FilteredSelectWindowClass(std::vector<std::wstring> std_string_list,
		std::vector<T> values,
		std::function<void(T*)> on_done,
		QWidget* parent,
		std::function<void(T*)> on_delete_function = nullptr) : BaseSelectorWidget<T, QListView, QSortFilterProxyModel>(nullptr, parent),
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

	}

	virtual void on_delete(const QModelIndex& source_index, const QModelIndex& selected_index) override {
		if (on_delete_function) {
			on_delete_function(&values[source_index.row()]);
			int delete_row = selected_index.row();
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

class CommandSelector : public BaseSelectorWidget<std::string, QTableView, QSortFilterProxyModel> {
private:
	QStringList string_elements;
	QStandardItemModel* standard_item_model = nullptr;
	std::unordered_map<std::string, std::vector<std::string>> key_map;
	std::function<void(std::string)>* on_done = nullptr;

	QList<QStandardItem*> get_item(std::string command_name) {

		std::string command_key = "";

		if (key_map.find(command_name) != key_map.end()) {
			const std::vector<std::string>& command_keys = key_map[command_name];
			for (int i = 0; i < command_keys.size(); i++) {
				const std::string& ck = command_keys[i];
				if (i > 0) {
					command_key += " | ";
				}
				command_key += ck;
			}

		}
		QStandardItem* name_item = new QStandardItem(QString::fromStdString(command_name));
		QStandardItem* key_item = new QStandardItem(QString::fromStdString(command_key));
		key_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
		return (QList<QStandardItem*>() << name_item << key_item);
	}

	QStandardItemModel* get_standard_item_model(std::vector<std::string> command_names) {

		QStandardItemModel* res = new QStandardItemModel();

		for (int i = 0; i < command_names.size(); i++) {
			res->appendRow(get_item(command_names[i]));
		}
		return res;
	}

	QStandardItemModel* get_standard_item_model(QStringList command_names) {

		std::vector<std::string> std_command_names;

		for (int i = 0; i < command_names.size(); i++) {
			std_command_names.push_back(command_names.at(i).toStdString());
		}
		return get_standard_item_model(std_command_names);
	}

protected:
public:

	QString get_view_stylesheet_type_name() {
		return "QTableView";
	}

	void on_select(const QModelIndex& index) {
		QString name = standard_item_model->data(index).toString();
		hide();
		parentWidget()->setFocus();
		(*on_done)(name.toStdString());
	}

	CommandSelector(std::function<void(std::string)>* on_done,
		QWidget* parent,
		QStringList elements,
		std::unordered_map<std::string,
		std::vector<std::string>> key_map) : BaseSelectorWidget<std::string, QTableView, QSortFilterProxyModel>(nullptr, parent),
		on_done(on_done),
		key_map(key_map)
	{
		string_elements = elements;
		standard_item_model = get_standard_item_model(string_elements);

		QTableView* table_view = dynamic_cast<QTableView*>(get_view());

		table_view->setSelectionMode(QAbstractItemView::SingleSelection);
		table_view->setSelectionBehavior(QAbstractItemView::SelectRows);
		table_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
		table_view->setModel(standard_item_model);

		table_view->horizontalHeader()->setStretchLastSection(true);
		table_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		table_view->horizontalHeader()->hide();
		table_view->verticalHeader()->hide();

	}

	virtual void on_return_no_select(const QString& text) {
		hide();
		parentWidget()->setFocus();
		(*on_done)(line_edit->text().toStdString());
	}

	virtual bool on_text_change(const QString& text) {

		std::vector<std::string> matching_element_names;
		std::vector<int> scores;
		std::string search_text_string = text.toStdString();
		std::vector<std::pair<std::string, int>> match_score_pairs;

		for (int i = 0; i < string_elements.size(); i++) {
			std::string encoded = utf8_encode(string_elements.at(i).toStdWString());
			int score = 0;
			fts::fuzzy_match(search_text_string.c_str(), encoded.c_str(), score);
			match_score_pairs.push_back(std::make_pair(encoded, score));
		}
		std::sort(match_score_pairs.begin(), match_score_pairs.end(), [](std::pair<std::string, int> lhs, std::pair<std::string, int> rhs) {
			return lhs.second > rhs.second;
			});

		for (int i = 0; i < string_elements.size(); i++) {
			if (string_elements.at(i).startsWith(text)) {
				matching_element_names.push_back(string_elements.at(i).toStdString());
			}
		}

		if (matching_element_names.size() == 0) {
			for (auto [command, _] : match_score_pairs) {
				matching_element_names.push_back(command);
			}
		}

		QStandardItemModel* new_standard_item_model = get_standard_item_model(matching_element_names);
		dynamic_cast<QTableView*>(get_view())->setModel(new_standard_item_model);
		delete standard_item_model;
		standard_item_model = new_standard_item_model;
		return true;
	}

};

class FileSelector : public BaseSelectorWidget<std::wstring, QListView, QSortFilterProxyModel> {
private:

	QStringListModel* list_model = nullptr;
	std::function<void(std::wstring)> on_done = nullptr;
	QString last_root = "";

protected:

public:

	QString get_view_stylesheet_type_name() {
		return "QListView";
	}

	FileSelector(std::function<void(std::wstring)> on_done, QWidget* parent, QString last_path) :
		BaseSelectorWidget<std::wstring, QListView, QSortFilterProxyModel>(nullptr, parent),
		on_done(on_done)
	{


		QString root_path;

		if (last_path.size() > 0) {
			QString file_name;
			split_root_file(last_path, root_path, file_name);
			root_path += QDir::separator();
		}

		list_model = new QStringListModel(get_dir_contents(root_path, ""));
		last_root = root_path;
		line_edit->setText(last_root);

		dynamic_cast<QListView*>(get_view())->setModel(list_model);
	}

	virtual bool on_text_change(const QString& text) {
		QString root_path;
		QString partial_name;
		split_root_file(text, root_path, partial_name);

		last_root = root_path;
		if (last_root.size() > 0) {
			if (last_root.at(last_root.size()-1) == QDir::separator()) {
				last_root.chop(1);
			}
		}

		QStringList match_list = get_dir_contents(root_path, partial_name);
		QStringListModel* new_list_model = new QStringListModel(match_list);
		dynamic_cast<QListView*>(get_view())->setModel(new_list_model);
		delete list_model;
		list_model = new_list_model;
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
				fts::fuzzy_match(encoded_prefix.c_str(), encoded_file.c_str(), score);
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
		QString full_path = expand_home_dir(last_root + sep + name);

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

std::wstring select_document_file_name();
std::wstring select_json_file_name();
std::wstring select_new_json_file_name();
std::wstring select_new_pdf_file_name();
