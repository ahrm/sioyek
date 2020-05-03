#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include <filesystem>

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


#include <Windows.h>
#include "utils.h"
#include "config.h"

using namespace std;

const int max_select_size = 100;


class ConfigFileChangeListener {
	static vector<ConfigFileChangeListener*> registered_listeners;


public:
	ConfigFileChangeListener();
	~ConfigFileChangeListener();
	virtual void on_config_file_changed(ConfigManager* new_config_manager) = 0;
	static void notify_config_file_changed(ConfigManager* new_config_manager);
};

template<typename T>
class FilteredSelectWindowClass : public QWidget, ConfigFileChangeListener{
private:

	QStringListModel* string_list_model;
	QSortFilterProxyModel* proxy_model;
	QLineEdit* line_edit;
	QListView* list_view;
	vector<T> values;
	function<void(void*)> on_done;
	ConfigManager* config_manager;

protected:
	bool eventFilter(QObject* obj, QEvent* event) override {
		if (obj == line_edit) {
			if (event->type() == QEvent::KeyPress) {
				QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
				auto kkk = key_event->key();
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
		wstring item_list_stylesheet = *new_config_manager->get_config<wstring>(L"item_list_stylesheet");
		wstring item_list_selected_stylesheet = *new_config_manager->get_config<wstring>(L"item_list_selected_stylesheet");

		//list_view->setStyleSheet(QString::fromStdWString(item_list_stylesheet));
		//list_view->setStyleSheet(QString::fromStdWString(item_list_selected_stylesheet));

		setStyleSheet(QString::fromStdWString(item_list_stylesheet));
		list_view->setStyleSheet("QListView::item::selected{" + QString::fromStdWString(item_list_selected_stylesheet) + "}");
	}

	//todo: check for memory leaks
	FilteredSelectWindowClass(vector<wstring> std_string_list, vector<T> values, function<void(void*)> on_done, ConfigManager* config_manager, QWidget* parent ) : 
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
		cout << "activated " << index.data().toString().toStdString() << endl;
		hide();
		parentWidget()->setFocus();
		auto source_index = proxy_model->mapToSource(index);
		on_done(&values[source_index.row()]);
	}
};

bool select_pdf_file_name(wchar_t* out_file_name, int max_length);
