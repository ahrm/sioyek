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
#include "imgui.h"

using namespace std;

const int max_select_size = 100;

class UIWidget {
public:
	virtual bool render() = 0;
};

template<typename T>
class FilteredSelect : public UIWidget{
private:
	vector<wstring> options;
	vector<T> values;
	function<void(void*)> on_done;
	int current_index;
	bool is_done;
	char select_string[max_select_size];

public:
	FilteredSelect(vector<wstring> options, vector<T> values, function<void(void*)> on_done) :
		options(options), values(values), current_index(0), is_done(false), on_done(on_done) {
		ZeroMemory(select_string, sizeof(select_string));
	}

	T* get_value() {
		int index = get_selected_index();
		if (index >= 0 && index < values.size()) {
			return &values[get_selected_index()];
		}
		return nullptr;
	}

	bool is_string_comppatible(wstring incomplete_string, wstring option_string) {
		incomplete_string = to_lower(incomplete_string);
		option_string = to_lower(option_string);
		return option_string.find(incomplete_string) < option_string.size();
	}

	int get_selected_index() {
		int index = -1;
		for (int i = 0; i < options.size(); i++) {
			if (is_string_comppatible(utf8_decode(select_string), options[i])) {
				index += 1;
			}
			if (index == current_index) {
				return i;
			}
		}
		return index;
	}

	wstring get_selected_option() {
		return options[get_selected_index()];
	}

	int get_max_index() {
		int max_index = -1;
		for (int i = 0; i < options.size(); i++) {
			if (is_string_comppatible(utf8_decode(select_string), options[i])) {
				max_index += 1;
			}
		}
		return max_index;
	}

	void move_item(int offset) {
		int new_index = offset + current_index;
		if (new_index < 0) {
			new_index = 0;
		}
		int max_index = get_max_index();
		if (new_index > max_index) {
			new_index = max_index;
		}
		current_index = new_index;
	}

	void next_item() {
		move_item(1);
	}

	void prev_item() {
		move_item(-1);
	}

	bool render() {
		ImGui::Begin("Select");

		ImGui::SetKeyboardFocusHere();
		if (ImGui::InputText("search string", select_string, sizeof(select_string), ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_EnterReturnsTrue,
			[](ImGuiInputTextCallbackData* data) {
				FilteredSelect* filtered_select = (FilteredSelect*)data->UserData;

				if (data->EventKey == ImGuiKey_UpArrow) {
					filtered_select->prev_item();
				}
				if (data->EventKey == ImGuiKey_DownArrow) {
					filtered_select->next_item();
				}

				return 0;
			}, this)) {
			if (is_done == false) {
				on_done(get_value());
			}
			is_done = true;
		}
		//ImGui::InputText("search", text_buffer, 100, ImGuiInputTextFlags_CallbackHistory, [&](ImGuiInputTextCallbackData* data) {

		int index = 0;
		for (int i = 0; i < options.size(); i++) {
			//if (options[i].find(select_string) == 0) {
			if (is_string_comppatible(utf8_decode(select_string), options[i])) {

				if (current_index == index) {
					ImGui::SetScrollHere();
				}

				if (ImGui::Selectable(utf8_encode(options[i]).c_str(), current_index == index)) {
					cout << "selected_something" << endl;
				}
				index += 1;
			}
		}

		ImGui::End();
		return is_done;
	}
};

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
