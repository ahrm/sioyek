#include "ui.h"
#include <qfiledialog.h>

#include <QItemSelectionModel>
#include <QTapGesture>
#include <main_widget.h>

extern std::wstring DEFAULT_OPEN_FILE_PATH;

std::wstring select_command_file_name(std::string command_name) {
	if (command_name == "open_document") {
		return select_document_file_name();
	}
	else if (command_name == "source_config") {
		return select_any_file_name();
	}
	else {
		return select_any_file_name();
	}
}

std::wstring select_document_file_name() {
	if (DEFAULT_OPEN_FILE_PATH.size() == 0) {

		QString file_name = QFileDialog::getOpenFileName(nullptr, "Select Document", "", "Documents (*.pdf *.epub *.cbz)");
		return file_name.toStdWString();
	}
	else {

		QFileDialog fd = QFileDialog(nullptr, "Select Document", "", "Documents (*.pdf *.epub *.cbz)");
		fd.setDirectory(QString::fromStdWString(DEFAULT_OPEN_FILE_PATH));
		if (fd.exec()) {
			
			QString file_name = fd.selectedFiles().first();
			return file_name.toStdWString();
		}
		else {
			return L"";
		}
	}

}

std::wstring select_json_file_name() {
	QString file_name = QFileDialog::getOpenFileName(nullptr, "Select Document", "", "Documents (*.json )");
	return file_name.toStdWString();
}

std::wstring select_any_file_name() {
	QString file_name = QFileDialog::getOpenFileName(nullptr, "Select File", "", "Any (*)");
	return file_name.toStdWString();
}

std::wstring select_new_json_file_name() {
	QString file_name = QFileDialog::getSaveFileName(nullptr, "Select Document", "", "Documents (*.json )");
	return file_name.toStdWString();
}

std::wstring select_new_pdf_file_name() {
	QString file_name = QFileDialog::getSaveFileName(nullptr, "Select Document", "", "Documents (*.pdf )");
	return file_name.toStdWString();
}


std::vector<ConfigFileChangeListener*> ConfigFileChangeListener::registered_listeners;

ConfigFileChangeListener::ConfigFileChangeListener() {
	registered_listeners.push_back(this);
}

ConfigFileChangeListener::~ConfigFileChangeListener() {
	registered_listeners.erase(std::find(registered_listeners.begin(), registered_listeners.end(), this));
}

void ConfigFileChangeListener::notify_config_file_changed(ConfigManager* new_config_manager) {
	for (auto* it : ConfigFileChangeListener::registered_listeners) {
		it->on_config_file_changed(new_config_manager);
	}
}

bool HierarchialSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
	// custom behaviour :
#ifdef SIOYEK_QT6
	if (filterRegularExpression().pattern().size() == 0)
#else
	if (filterRegExp().isEmpty() == false)
#endif
	{
		// get source-model index for current row
		QModelIndex source_index = sourceModel()->index(source_row, this->filterKeyColumn(), source_parent);
		if (source_index.isValid())
		{
			// check current index itself :
			QString key = sourceModel()->data(source_index, filterRole()).toString();

#ifdef SIOYEK_QT6
			bool parent_contains = key.contains(filterRegularExpression());
#else
			bool parent_contains = key.contains(filterRegExp());
#endif

			if (parent_contains) return true;

			// if any of children matches the filter, then current index matches the filter as well
			int i, nb = sourceModel()->rowCount(source_index);
			for (i = 0; i < nb; ++i)
			{
				if (filterAcceptsRow(i, source_index))
				{
					return true;
				}
			}
			return false;
		}
	}
	// parent call for initial behaviour
	return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

bool MySortFilterProxyModel::filterAcceptsRow(int source_row,
	const QModelIndex& source_parent) const
{
	if (FUZZY_SEARCHING) {

		QModelIndex source_index = sourceModel()->index(source_row, this->filterKeyColumn(), source_parent);
		if (source_index.isValid())
		{
			// check current index itself :

			QString key = sourceModel()->data(source_index, filterRole()).toString();
			if (filterString.size() == 0) return true;
			std::wstring s1 = filterString.toStdWString();
			std::wstring s2 = key.toStdWString();
			int score = static_cast<int>(rapidfuzz::fuzz::partial_ratio(s1, s2));

			return score > 50;
		}
		else {
			return false;
		}
	}
	else {
		return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
	}
}

void MySortFilterProxyModel::setFilterCustom(QString filterString) {
	if (FUZZY_SEARCHING) {
		this->filterString = filterString;
		this->setFilterFixedString(filterString);
		sort(0);
	}
	else {
		this->setFilterFixedString(filterString);
	}
}

bool MySortFilterProxyModel::lessThan(const QModelIndex& left,
	const QModelIndex& right) const
{
	if (FUZZY_SEARCHING) {

		QString leftData = sourceModel()->data(left).toString();
		QString rightData = sourceModel()->data(right).toString();

		int left_score = static_cast<int>(rapidfuzz::fuzz::partial_ratio(filterString.toStdWString(), leftData.toStdWString()));
		int right_score = static_cast<int>(rapidfuzz::fuzz::partial_ratio(filterString.toStdWString(), rightData.toStdWString()));
		return left_score > right_score;
	}
	else {
		return QSortFilterProxyModel::lessThan(left, right);
	}
}
 MySortFilterProxyModel::MySortFilterProxyModel() {
	 if (FUZZY_SEARCHING) {
		 setDynamicSortFilter(true);
	 }
}

 AndroidSelector::AndroidSelector(QWidget* parent) : QWidget(parent){
     layout = new QVBoxLayout();

     main_widget = dynamic_cast<MainWidget*>(parent);


    fullscreen_button = new QPushButton("Fullscreen");
    select_text_button = new QPushButton("Select Text");
    open_document_button = new QPushButton("Open Document");
    command_button = new QPushButton("Command");

    layout->addWidget(fullscreen_button);
    layout->addWidget(select_text_button);
    layout->addWidget(open_document_button);
    layout->addWidget(command_button);

    QObject::connect(fullscreen_button, &QPushButton::pressed, [&](){
        main_widget->toggle_fullscreen();
    });

    QObject::connect(select_text_button, &QPushButton::pressed, [&](){
        main_widget->handle_mobile_selection();
        main_widget->invalidate_render();
    });

    QObject::connect(open_document_button, &QPushButton::pressed, [&](){
        auto command = main_widget->command_manager->get_command_with_name("open_document");
        main_widget->handle_command_types(std::move(command), 0);
    });

    QObject::connect(command_button, &QPushButton::pressed, [&](){
        auto command = main_widget->command_manager->get_command_with_name("command");
        main_widget->handle_command_types(std::move(command), 0);
    });

     layout->insertStretch(-1, 1);

     this->setLayout(layout);
 }

void AndroidSelector::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    setFixedSize(parent_width * 0.9f, parent_height);
//    list_view->setFixedSize(parent_width * 0.9f, parent_height);
    move(parent_width * 0.05f, 0);

}
