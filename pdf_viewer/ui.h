#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <functional>

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
#include <qstringlistmodel.h>
#include <qpalette.h>
#include <qstandarditemmodel.h>


//#include <Windows.h>
#include "utils.h"
#include "config.h"

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
	bool eventFilter(QObject* obj, QEvent* event) override {
		if (obj == line_edit) {
			if (event->type() == QEvent::KeyPress) {
				QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
				if (key_event->key() == Qt::Key_Down ||
					key_event->key() == Qt::Key_Up ||
					key_event->key() == Qt::Key_Left ||
					key_event->key() == Qt::Key_Right ||
					key_event->key() == Qt::Key_Return) {
					if (key_event->key() == Qt::Key_Enter) {
						tree_view->setFocus();
					}
					QKeyEvent* newEvent = new QKeyEvent(*key_event);
					QCoreApplication::postEvent(tree_view, newEvent);
					//QCoreApplication::postEvent(tree_view, key_event);
					return true;
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
	FilteredTreeSelect(QStandardItemModel* item_model, std::function<void(const std::vector<int>&)> on_done, ConfigManager* config_manager, QWidget* parent ) : 
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
		layout->addWidget(line_edit);
		layout->addWidget(tree_view);

		line_edit->setFont(QFont("Monaco"));
		tree_view->setFont(QFont("Monaco"));

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

		setStyleSheet("background-color: black; color: white; border: 0;");
		tree_view->setStyleSheet("QTreeView::item::selected{background-color: white; color: black;}");

	}

	void resizeEvent(QResizeEvent* resize_event) override {
		QWidget::resizeEvent(resize_event);
		int parent_width = parentWidget()->width();
		int parent_height = parentWidget()->height();
		setFixedSize(parent_width / 3, parent_height);
		move(parent_width / 3, 0);
		on_config_file_changed(config_manager);
	}


	void on_select(const QModelIndex& index) {
		std::wcout << "activated " << endl;
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
	ConfigManager* config_manager = nullptr;

protected:
	bool eventFilter(QObject* obj, QEvent* event) override {
		if (obj == line_edit) {
			if (event->type() == QEvent::KeyPress) {
				QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
				if (key_event->key() == Qt::Key_Down || key_event->key() == Qt::Key_Up || key_event->key() == Qt::Key_Return) {
					QKeyEvent* new_key_event = new QKeyEvent(*key_event);
					if (key_event->key() == Qt::Key_Enter) {
						list_view->setFocus();
					}
					QCoreApplication::postEvent(list_view, new_key_event);
					return true;
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
	FilteredSelectWindowClass(std::vector<std::wstring> std_string_list, std::vector<T> values, std::function<void(T*)> on_done, ConfigManager* config_manager, QWidget* parent ) : 
		QWidget(parent) ,
		values(values),
		config_manager(config_manager),
		on_done(on_done)
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

		line_edit->setFont(QFont("Monaco"));
		list_view->setFont(QFont("Monaco"));

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

	void resizeEvent(QResizeEvent* resize_event) override {
		QWidget::resizeEvent(resize_event);
		int parent_width = parentWidget()->width();
		int parent_height = parentWidget()->height();
		setFixedSize(parent_width / 3, parent_height);
		move(parent_width / 3, 0);
		//list_view->setStyleSheet("QListView{ background-color: black;color: white; }");
		//list_view->setStyleSheet(*config_manager->get_config<string>("item_list_stylesheet"));
		//list_view->setStyleSheet(*config_manager->get_config<string>("item_list_selected_stylesheet"));
		//list_view->setStyleSheet("QListView::item::selected{ background-color: white;color: black; }");
		on_config_file_changed(config_manager);
	}


	void on_select(const QModelIndex& index) {
		std::wcout << "activated " << index.data().toString().toStdWString() << std::endl;
		hide();
		parentWidget()->setFocus();
		auto source_index = proxy_model->mapToSource(index);
		on_done(&values[source_index.row()]);
	}
};

std::wstring select_document_file_name();
//bool select_document_file_name(wchar_t* out_file_name, int max_length);

