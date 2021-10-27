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


//#include <Windows.h>
#include "utils.h"
#include "config.h"

extern std::wstring UI_FONT_FACE_NAME;
const int max_select_size = 100;

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

template<typename T>
class FilteredTreeSelect : public QWidget, ConfigFileChangeListener{
private:

	QStandardItemModel* tree_item_model = nullptr;
	QSortFilterProxyModel* proxy_model = nullptr;
	QLineEdit* line_edit = nullptr;
	QTreeView* tree_view = nullptr;
	std::function<void(const std::vector<int>&)> on_done;
	ConfigManager* config_manager = nullptr;

protected:

	std::optional<QModelIndex> get_selected_index() {
		QModelIndexList selected_index_list = tree_view->selectionModel()->selectedIndexes();

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
				if (key_event->key() == Qt::Key_Down ||
					key_event->key() == Qt::Key_Up ||
					key_event->key() == Qt::Key_Left ||
					key_event->key() == Qt::Key_Right
					) {
					QKeyEvent* newEvent = new QKeyEvent(*key_event);
					QCoreApplication::postEvent(tree_view, newEvent);
					//QCoreApplication::postEvent(tree_view, key_event);
					return true;
				}
				if (key_event->key() == Qt::Key_Tab) {
					QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Down, key_event->modifiers());
					QCoreApplication::postEvent(tree_view, new_key_event);
					return true;
				}
				if (key_event->key() == Qt::Key_Backtab) {
					QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Up, key_event->modifiers());
					QCoreApplication::postEvent(tree_view, new_key_event);
					return true;
				}
				if (key_event->key() == Qt::Key_Return || key_event->key() == Qt::Key_Enter) {
					std::optional<QModelIndex> selected_index = get_selected_index();
					if (selected_index) {
						on_select(selected_index.value());
					}
				}

			}
		}
		return false;
	}

public:

	void on_config_file_changed(ConfigManager* new_config_manager) {
		setStyleSheet("background-color: black; color: white; border: 0;");
		tree_view->setStyleSheet("QTreeView::item::selected{background-color: white; color: black;}");
	}

	//todo: check for memory leaks
	FilteredTreeSelect(QStandardItemModel* item_model,
		std::function<void(const std::vector<int>&)> on_done,
		ConfigManager* config_manager,
		QWidget* parent,
		std::vector<int> selected_index) : 
		QWidget(parent) ,
		tree_item_model(item_model),
		config_manager(config_manager),
		on_done(on_done)
	{

		proxy_model = new HierarchialSortFilterProxyModel;
		//proxy_model = new QSortFilterProxyModel;
		//proxy_model->setRecursiveFilteringEnabled(true);
		proxy_model->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
		proxy_model->setSourceModel(tree_item_model);

		resize(300, 800);
		QVBoxLayout* layout = new QVBoxLayout;
		setLayout(layout);

		line_edit = new QLineEdit;
		tree_view = new QTreeView;
		tree_view->setModel(proxy_model);
		tree_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
		tree_view->expandAll();
		tree_view->setHeaderHidden(true);
		tree_view->resizeColumnToContents(0);

		auto index = QModelIndex();
		for (auto i : selected_index) {
			index = proxy_model->index(i, 0, index);
		}

		tree_view->setCurrentIndex(index);

		layout->addWidget(line_edit);
		layout->addWidget(tree_view);

		line_edit->setFont(QFont(QString::fromStdWString(UI_FONT_FACE_NAME)));
		tree_view->setFont(QFont(QString::fromStdWString(UI_FONT_FACE_NAME)));

		line_edit->installEventFilter(this);
		line_edit->setFocus();

		QObject::connect(tree_view, &QAbstractItemView::activated, [&](const QModelIndex& index) {
			on_select(index);
			});

		QObject::connect(line_edit, &QLineEdit::textChanged, [&](const QString& text) {
			//proxy_model->setFilterRegExp(text);
			proxy_model->setFilterFixedString(text);
			//std::cout << ((HierarchialSortFilterProxyModel*)proxy_model)->count << "\n";
			tree_view->expandAll();
			});

		//setStyleSheet("background-color: black; color: white; border: 0; QScrollBar::vertical{background: red;}");
		//tree_view->setStyleSheet("QTreeView::item::selected{background-color: white; color: black;}");

	}

	void resizeEvent(QResizeEvent* resize_event) override {
		QWidget::resizeEvent(resize_event);
		int parent_width = parentWidget()->width();
		int parent_height = parentWidget()->height();
		//setFixedSize(parent_width / 3, parent_height);
		//move(parent_width / 3, 0);
		setFixedSize(parent_width * 0.9f, parent_height);
		move(parent_width * 0.05f, 0);
		on_config_file_changed(config_manager);
	}


	void on_select(const QModelIndex& index) {
		hide();
		parentWidget()->setFocus();
		auto source_index = proxy_model->mapToSource(index);
		//auto parent = source_index.parent();
		//parent = source_index.parent();
		//parent = source_index.parent();
		//parent = source_index.parent();
		//parent = source_index.parent();
		std::vector<int> indices;
		while (source_index != QModelIndex()) {
			indices.push_back(source_index.row());
			source_index = source_index.parent();
		}
		on_done(indices);
	}
};

template<typename T>
class FilteredSelectWindowClass : public QWidget, ConfigFileChangeListener{
private:

	QStringListModel* string_list_model = nullptr;
	QSortFilterProxyModel* proxy_model = nullptr;
	QLineEdit* line_edit = nullptr;
	QListView* list_view = nullptr;
	std::vector<T> values;
	//std::function<void(void*)> on_done = nullptr;
	std::function<void(T*)> on_done = nullptr;
	std::function<void(T*)> on_delete = nullptr;
	ConfigManager* config_manager = nullptr;

protected:
	std::optional<QModelIndex> get_selected_index() {
		QModelIndexList selected_index_list = list_view->selectionModel()->selectedIndexes();

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
				if (key_event->key() == Qt::Key_Down || key_event->key() == Qt::Key_Up) {
					QKeyEvent* new_key_event = new QKeyEvent(*key_event);
					QCoreApplication::postEvent(list_view, new_key_event);
					return true;
				}
				if (key_event->key() == Qt::Key_Tab) {
					QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Down, key_event->modifiers());
					QCoreApplication::postEvent(list_view, new_key_event);
					return true;
				}
				if (key_event->key() == Qt::Key_Backtab) {
					QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Up, key_event->modifiers());
					QCoreApplication::postEvent(list_view, new_key_event);
					return true;
				}

				if (key_event->key() == Qt::Key_Enter || key_event->key() == Qt::Key_Return) {
					//QModelIndexList selected_index_list = list_view->selectionModel()->selectedIndexes();
					//if (selected_index_list.size() > 0) {
					//	QModelIndex selected_index = selected_index_list.at(0);
					//	QModelIndex source_index = proxy_model->mapToSource(selected_index);

					//	on_delete(&values[source_index.row()]);

					//	int delete_row = selected_index.row();
					//	proxy_model->removeRow(selected_index.row());
					//	values.erase(values.begin() + source_index.row());
					//}
					std::optional<QModelIndex> selected_index = get_selected_index();
					if (selected_index) {
						on_select(selected_index.value());
					}
					//on_select(proxy_model->index());
				}

			}
		}
		return false;
	}

public:

	void on_config_file_changed(ConfigManager* new_config_manager) {
		setStyleSheet("background-color: black; color: white; border: 0;");
		list_view->setStyleSheet("QListView::item::selected{background-color: white; color: black;}");
	}

	//todo: check for memory leaks
	//FilteredSelectWindowClass(std::vector<std::wstring> std_string_list, std::vector<T> values, std::function<void(void*)> on_done, ConfigManager* config_manager, QWidget* parent ) : 
	FilteredSelectWindowClass(std::vector<std::wstring> std_string_list,
		std::vector<T> values,
		std::function<void(T*)> on_done,
		ConfigManager* config_manager,
		QWidget* parent,
		std::function<void(T*)> on_delete=nullptr) :
		QWidget(parent) ,
		values(values),
		config_manager(config_manager),
		on_done(on_done),
		on_delete(on_delete)
	{
		QVector<QString> q_string_list;
		for (const auto& s : std_string_list) {
			q_string_list.push_back(QString::fromStdWString(s));

		}
		QStringList string_list = QStringList::fromVector(q_string_list);

		proxy_model = new QSortFilterProxyModel;
		proxy_model->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
		string_list_model = new QStringListModel(string_list);
		proxy_model->setSourceModel(string_list_model);

		resize(300, 800);
		QVBoxLayout* layout = new QVBoxLayout;
		setLayout(layout);

		line_edit = new QLineEdit;
		list_view = new QListView;
		list_view->setModel(proxy_model);
		list_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
		layout->addWidget(line_edit);
		layout->addWidget(list_view);

		line_edit->setFont(QFont(QString::fromStdWString(UI_FONT_FACE_NAME)));
		list_view->setFont(QFont(QString::fromStdWString(UI_FONT_FACE_NAME)));

		//line_edit->setStyleSheet("background-color: yellow;");
		//setStyleSheet("background-color: black;color: white; border: 0;");

		//palette.setColor(Qpalette);
		//setPalette(palette);

		//line_edit->setAutoFillBackground(true);
		//line_edit->setPalette(palette);
		//line_edit->setStyleSheet("background-color: black; color: white; border: 0;");
		//list_view->setStyleSheet("background-color: black; color: white;");
		//list_view->setPalette(palette);

		//setStyleSheet("background-color: black; color: white;");
		//list_view->setStyleSheet("color: red");
		//setFont(QFont("Monaco"));

		line_edit->installEventFilter(this);
		line_edit->setFocus();

		QObject::connect(list_view, &QAbstractItemView::activated, [&](const QModelIndex& index) {
			on_select(index);
			});


		QObject::connect(line_edit, &QLineEdit::textChanged, [&](const QString& text) {
			//proxy_model->setFilterRegExp(text);
			proxy_model->setFilterFixedString(text);
			});

		//setStyleSheet(QString::fromStdWString(item_list_stylesheet));
		//list_view->setStyleSheet("QListView::item::selected{" + QString::fromStdWString(item_list_selected_stylesheet) + "}");

		setStyleSheet("background-color: black; color: white; border: 0;");
		list_view->setStyleSheet("QListView::item::selected{background-color: white; color: black;}");

	}

	void keyReleaseEvent(QKeyEvent* event) override {
		if (event->key() == Qt::Key_Delete) {
			if (on_delete != nullptr) { // only handle delete when the user has provided the `on_delete` function
				QModelIndexList selected_index_list = list_view->selectionModel()->selectedIndexes();
				if (selected_index_list.size() > 0) {
					QModelIndex selected_index = selected_index_list.at(0);
					QModelIndex source_index = proxy_model->mapToSource(selected_index);

					on_delete(&values[source_index.row()]);

					int delete_row = selected_index.row();
					proxy_model->removeRow(selected_index.row());
					values.erase(values.begin() + source_index.row());
				}
			}
		}
		QWidget::keyReleaseEvent(event);
	}

	void resizeEvent(QResizeEvent* resize_event) override {
		QWidget::resizeEvent(resize_event);
		int parent_width = parentWidget()->width();
		int parent_height = parentWidget()->height();
		setFixedSize(parent_width * 0.9f, parent_height);
		move(parent_width * 0.05f, 0);
		on_config_file_changed(config_manager);
	}


	void on_select(const QModelIndex& index) {
		hide();
		parentWidget()->setFocus();
		auto source_index = proxy_model->mapToSource(index);
		on_done(&values[source_index.row()]);
	}
};

class CommandSelector : public QWidget{
private:

	QLineEdit* line_edit = nullptr;
	QTableView* table_view = nullptr;
	QStringList string_elements;
	//QStringListModel* list_model = nullptr;
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
	std::optional<QModelIndex> get_selected_index() {
		QModelIndexList selected_index_list = table_view->selectionModel()->selectedIndexes();

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
				if (key_event->key() == Qt::Key_Down || key_event->key() == Qt::Key_Up) {
					QKeyEvent* new_key_event = new QKeyEvent(*key_event);
					QCoreApplication::postEvent(table_view, new_key_event);
					return true;
				}
				if (key_event->key() == Qt::Key_Tab) {
					QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Down, key_event->modifiers());
					QCoreApplication::postEvent(table_view, new_key_event);
					return true;
				}
				if (key_event->key() == Qt::Key_Backtab) {
					QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Up, key_event->modifiers());
					QCoreApplication::postEvent(table_view, new_key_event);
					return true;
				}

				if (key_event->key() == Qt::Key_Enter || key_event->key() == Qt::Key_Return) {
					std::optional<QModelIndex> selected_index = get_selected_index();
					if (selected_index) {
						on_select(selected_index.value());
					}
					else {
						hide();
						parentWidget()->setFocus();
						(*on_done)(line_edit->text().toStdString());
					}
				}

			}
		}
		return false;
	}

public:

	void on_config_file_changed() {
		setStyleSheet("background-color: black; color: white; border: 0;");
		table_view->setStyleSheet("QTableView::item::selected{background-color: white; color: black;}");
		//myTable->setStyleSheet("QHeaderView {background-color: transparent;}");
		table_view->horizontalHeader()->setStretchLastSection(true);
		table_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		table_view->horizontalHeader()->hide();
		table_view->verticalHeader()->hide();

	}

	CommandSelector(std::function<void(std::string)>* on_done, QWidget* parent, QStringList elements, std::unordered_map<std::string, std::vector<std::string>> key_map) :
		QWidget(parent) ,
		on_done(on_done),
		key_map(key_map)
	{
		resize(300, 800);
		QVBoxLayout* layout = new QVBoxLayout;
		setLayout(layout);

		string_elements = elements;
		//list_model = new QStringListModel(string_elements);
		standard_item_model = get_standard_item_model(string_elements);

		line_edit = new QLineEdit;
		table_view = new QTableView;
		table_view->setSelectionMode(QAbstractItemView::SingleSelection);
		table_view->setSelectionBehavior(QAbstractItemView::SelectRows);
		table_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
		table_view->setModel(standard_item_model);
		layout->addWidget(line_edit);
		layout->addWidget(table_view);

		line_edit->installEventFilter(this);
		line_edit->setFocus();

		QObject::connect(table_view, &QAbstractItemView::activated, [&](const QModelIndex& index) {
			on_select(index);
			});


		QObject::connect(line_edit, &QLineEdit::textChanged, [&](const QString& text) {

			std::vector<std::string> matching_element_names;

			for (int i = 0; i < string_elements.size(); i++) {
				if (string_elements.at(i).startsWith(text)) {
					matching_element_names.push_back(string_elements.at(i).toStdString());
				}
			}
			//QStringListModel* new_standard_item_model = new QStringListModel(matching_elements);

			QStandardItemModel* new_standard_item_model = get_standard_item_model(matching_element_names);
			table_view->setModel(new_standard_item_model);
			delete standard_item_model;
			standard_item_model = new_standard_item_model;
			});


		on_config_file_changed();
	}

	void resizeEvent(QResizeEvent* resize_event) override {
		QWidget::resizeEvent(resize_event);
		int parent_width = parentWidget()->width();
		int parent_height = parentWidget()->height();
		setFixedSize(parent_width * 0.9f, parent_height);
		move(parent_width * 0.05f, 0);
		on_config_file_changed();
	}


	void on_select(const QModelIndex& index) {

		QString name = standard_item_model->data(index).toString();
		hide();
		parentWidget()->setFocus();
		(*on_done)(name.toStdString());
	}
};

class FileSelector : public QWidget{
private:

	QLineEdit* line_edit = nullptr;
	QListView* list_view = nullptr;
	QStringListModel* list_model = nullptr;
	std::function<void(std::wstring)> on_done = nullptr;
	QString last_root = "";

protected:
	std::optional<QModelIndex> get_selected_index() {
		QModelIndexList selected_index_list = list_view->selectionModel()->selectedIndexes();

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
				if (key_event->key() == Qt::Key_Down || key_event->key() == Qt::Key_Up) {
					QKeyEvent* new_key_event = new QKeyEvent(*key_event);
					QCoreApplication::postEvent(list_view, new_key_event);
					return true;
				}
				if (key_event->key() == Qt::Key_Tab) {
					QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Down, key_event->modifiers());
					QCoreApplication::postEvent(list_view, new_key_event);
					return true;
				}
				if (key_event->key() == Qt::Key_Backtab) {
					QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Up, key_event->modifiers());
					QCoreApplication::postEvent(list_view, new_key_event);
					return true;
				}

				if (key_event->key() == Qt::Key_Enter || key_event->key() == Qt::Key_Return) {
					std::optional<QModelIndex> selected_index = get_selected_index();
					if (selected_index) {
						on_select(selected_index.value());
					}
				}

			}
		}
		return false;
	}

public:

	void on_config_file_changed() {
		setStyleSheet("background-color: black; color: white; border: 0;");
		list_view->setStyleSheet("QListView::item::selected{background-color: white; color: black;}");
	}

	//todo: check for memory leaks
	//FilteredSelectWindowClass(std::vector<std::wstring> std_string_list, std::vector<T> values, std::function<void(void*)> on_done, ConfigManager* config_manager, QWidget* parent ) : 
	FileSelector(std::function<void(std::wstring)> on_done, QWidget* parent) :
		QWidget(parent) ,
		on_done(on_done)
	{
		resize(300, 800);
		QVBoxLayout* layout = new QVBoxLayout;
		setLayout(layout);

		list_model = new QStringListModel(get_dir_contents("", ""));

		line_edit = new QLineEdit;
		list_view = new QListView;
		list_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
		list_view->setModel(list_model);
		layout->addWidget(line_edit);
		layout->addWidget(list_view);

		line_edit->installEventFilter(this);
		line_edit->setFocus();

		QObject::connect(list_view, &QAbstractItemView::activated, [&](const QModelIndex& index) {
			on_select(index);
			});


		QObject::connect(line_edit, &QLineEdit::textChanged, [&](const QString& text) {
			QString root_path;
			QString partial_name;
			split_root_file(text, root_path, partial_name);

			last_root = root_path;
			if (last_root.size() > 0) {
				if (last_root.back() == QDir::separator()) {
					last_root.chop(1);
				}
			}

			QStringListModel* new_list_model = new QStringListModel(get_dir_contents(root_path, partial_name));
			list_view->setModel(new_list_model);
			delete list_model;
			list_model = new_list_model;
			});


		setStyleSheet("background-color: black; color: white; border: 0;");
		list_view->setStyleSheet("QListView::item::selected{background-color: white; color: black;}");

	}
	QStringList get_dir_contents(QString root, QString prefix) {
		root = expand_home_dir(root);
		QDir directory(root);
		return directory.entryList({ prefix + "*" });
	}

	void resizeEvent(QResizeEvent* resize_event) override {
		QWidget::resizeEvent(resize_event);
		int parent_width = parentWidget()->width();
		int parent_height = parentWidget()->height();
		setFixedSize(parent_width * 0.9f, parent_height);
		move(parent_width * 0.05f, 0);
		on_config_file_changed();
	}


	void on_select(const QModelIndex& index) {
		//hide();

		QString name = list_model->data(index).toString();
		QChar sep = QDir::separator();
		QString full_path = expand_home_dir(last_root + sep + name);

		if (QFileInfo(full_path).isFile()){
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

