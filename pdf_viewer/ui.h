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


#include <Windows.h>
#include "utils.h"

using namespace std;

const int max_select_size = 100;

template<typename T>
class FilteredSelectWindowClass : public QWidget {
private:

	QStringListModel* string_list_model;
	QSortFilterProxyModel* proxy_model;
	QLineEdit* line_edit;
	QListView* list_view;
	vector<T> values;
	function<void(void*)> on_done;

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

	//todo: check for memory leaks
	FilteredSelectWindowClass(vector<wstring> std_string_list, vector<T> values, function<void(void*)> on_done,  QWidget* parent ) : 
		QWidget(parent) ,
		values(values),
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

		resize(500, 500);
		QVBoxLayout* layout = new QVBoxLayout;
		setLayout(layout);

		line_edit = new QLineEdit;
		list_view = new QListView;
		list_view->setModel(proxy_model);
		list_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
		layout->addWidget(line_edit);
		layout->addWidget(list_view);

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
	}

	//void set_values(QStringList string_list, vector<T> values_, function<void(void*)> on_done_) {
	//	values = values_;
	//	on_done_ = on_done_;
	//	string_list_model->setStringList(string_list);
	//}

	void on_select(const QModelIndex& index) {
		cout << "activated " << index.data().toString().toStdString() << endl;
		hide();
		parentWidget()->setFocus();
		auto source_index = proxy_model->mapToSource(index);
		on_done(&values[source_index.row()]);
	}
};

bool select_pdf_file_name(wchar_t* out_file_name, int max_length);
