#include "ui.h"
#include <qfiledialog.h>

#include <QItemSelectionModel>
#include <QTapGesture>
#include <main_widget.h>

extern std::wstring DEFAULT_OPEN_FILE_PATH;
extern float DARK_MODE_CONTRAST;
extern float BACKGROUND_COLOR[3];
extern bool RULER_MODE;

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

#ifdef SIOYEK_ANDROID
 AndroidSelector::AndroidSelector(QWidget* parent) : QWidget(parent){
     layout = new QVBoxLayout();

     main_widget = dynamic_cast<MainWidget*>(parent);


    fullscreen_button = new QPushButton("Fullscreen", this);
    select_text_button = new QPushButton("Select Text", this);
    open_document_button = new QPushButton("Open New Document", this);
    open_prev_document_button = new QPushButton("Open Previous Document", this);
    command_button = new QPushButton("Command", this);
    visual_mode_button = new QPushButton("Visual Mark Mode", this);
    search_button = new QPushButton("Search", this);
    set_background_color = new QPushButton("Background Color", this);
    set_dark_mode_contrast = new QPushButton("Dark Mode Contrast", this);
    set_ruler_mode = new QPushButton("Ruler Mode", this);
    restore_default_config_button = new QPushButton("Restore Default Config", this);

    layout->addWidget(fullscreen_button);
    layout->addWidget(select_text_button);
    layout->addWidget(open_document_button);
    layout->addWidget(open_prev_document_button);
    layout->addWidget(command_button);
    layout->addWidget(visual_mode_button);
    layout->addWidget(search_button);
    layout->addWidget(set_background_color);
    layout->addWidget(set_dark_mode_contrast);
    layout->addWidget(set_ruler_mode);
    layout->addWidget(restore_default_config_button);

    QObject::connect(fullscreen_button, &QPushButton::pressed, [&](){
        main_widget->current_widget = {};
        deleteLater();
        main_widget->toggle_fullscreen();
    });

    QObject::connect(select_text_button, &QPushButton::pressed, [&](){
        main_widget->current_widget = {};
        deleteLater();
        main_widget->handle_mobile_selection();
        main_widget->invalidate_render();
    });

    QObject::connect(open_document_button, &QPushButton::pressed, [&](){
        main_widget->current_widget = {};
        deleteLater();
        auto command = main_widget->command_manager->get_command_with_name("open_document");
        main_widget->handle_command_types(std::move(command), 0);
    });

    QObject::connect(open_prev_document_button, &QPushButton::pressed, [&](){
        auto command = main_widget->command_manager->get_command_with_name("open_prev_doc");
        main_widget->current_widget = {};
        deleteLater();
        main_widget->handle_command_types(std::move(command), 0);
    });

    QObject::connect(command_button, &QPushButton::pressed, [&](){
        auto command = main_widget->command_manager->get_command_with_name("command");
        main_widget->current_widget = {};
        deleteLater();
        main_widget->handle_command_types(std::move(command), 0);
    });

    QObject::connect(visual_mode_button, &QPushButton::pressed, [&](){
        main_widget->android_handle_visual_mode();
        main_widget->current_widget = {};
        deleteLater();
    });

    QObject::connect(search_button, &QPushButton::pressed, [&](){
        auto command = main_widget->command_manager->get_command_with_name("search");
        main_widget->current_widget = {};
        deleteLater();
        main_widget->handle_command_types(std::move(command), 0);
    });

    QObject::connect(set_background_color, &QPushButton::pressed, [&](){

        auto command = main_widget->command_manager->get_command_with_name("setconfig_background_color");
        main_widget->current_widget = {};
        deleteLater();
        main_widget->handle_command_types(std::move(command), 0);
    });

    QObject::connect(set_dark_mode_contrast, &QPushButton::pressed, [&](){

        main_widget->current_widget = {};
        deleteLater();

        main_widget->current_widget = new FloatConfigUI(main_widget, &DARK_MODE_CONTRAST, 0.0f, 1.0f);
        main_widget->current_widget->show();
    });

    QObject::connect(set_ruler_mode, &QPushButton::pressed, [&](){

        auto command = main_widget->command_manager->get_command_with_name("setconfig_ruler_mode");
        main_widget->current_widget = {};
        deleteLater();
        main_widget->handle_command_types(std::move(command), 0);

    });

    QObject::connect(restore_default_config_button, &QPushButton::pressed, [&](){

        main_widget->current_widget = {};
        deleteLater();
        main_widget->restore_default_config();

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

TextSelectionButtons::TextSelectionButtons(MainWidget* parent) : QWidget(parent) {

    QHBoxLayout* layout = new QHBoxLayout();

    main_widget = parent;
    copy_button = new QPushButton("Copy");
    search_in_scholar_button = new QPushButton("Search Scholar");
    search_in_google_button = new QPushButton("Search Google");
    highlight_button = new QPushButton("Highlight");

    QObject::connect(copy_button, &QPushButton::clicked, [&](){
        copy_to_clipboard(main_widget->selected_text);
    });

    QObject::connect(search_in_scholar_button, &QPushButton::clicked, [&](){
        search_custom_engine(main_widget->selected_text, L"https://scholar.google.com/scholar?&q=");
    });

    QObject::connect(search_in_google_button, &QPushButton::clicked, [&](){
        search_custom_engine(main_widget->selected_text, L"https://www.google.com/search?q=");
    });

    QObject::connect(highlight_button, &QPushButton::clicked, [&](){
        main_widget->handle_touch_highlight();
    });

    layout->addWidget(copy_button);
    layout->addWidget(search_in_scholar_button);
    layout->addWidget(search_in_google_button);
    layout->addWidget(highlight_button);

    this->setLayout(layout);
}

void TextSelectionButtons::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    setFixedSize(parent_width, parent_height / 5);
//    list_view->setFixedSize(parent_width * 0.9f, parent_height);
    move(0, 0);

}

HighlightButtons::HighlightButtons(MainWidget* parent) : QWidget(parent){
    main_widget = parent;
    layout = new QHBoxLayout();

    delete_highlight_button = new QPushButton("Delete");
    QObject::connect(delete_highlight_button, &QPushButton::clicked, [&](){
        main_widget->handle_delete_selected_highlight();
        hide();
        main_widget->highlight_buttons = nullptr;
        deleteLater();
    });

    layout->addWidget(delete_highlight_button);
    this->setLayout(layout);
}

void HighlightButtons::resizeEvent(QResizeEvent* resize_event){

    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    setFixedSize(parent_width, parent_height / 5);
    move(0, 0);
}

SearchButtons::SearchButtons(MainWidget* parent) : QWidget(parent){
    main_widget = parent;
    layout = new QHBoxLayout();

//    delete_highlight_button = new QPushButton("Delete");
//    QObject::connect(delete_highlight_button, &QPushButton::clicked, [&](){
//        main_widget->handle_delete_selected_highlight();
//        hide();
//        main_widget->highlight_buttons = nullptr;
//        deleteLater();
//    });
    next_match_button = new QPushButton("next");
    prev_match_button = new QPushButton("prev");
    goto_initial_location_button = new QPushButton("initial");

    QObject::connect(next_match_button, &QPushButton::clicked, [&](){
        main_widget->opengl_widget->goto_search_result(1);
        main_widget->invalidate_render();
    });

    QObject::connect(prev_match_button, &QPushButton::clicked, [&](){
        main_widget->opengl_widget->goto_search_result(-1);
        main_widget->invalidate_render();
    });

    QObject::connect(goto_initial_location_button, &QPushButton::clicked, [&](){
        main_widget->goto_mark('/');
        main_widget->invalidate_render();
    });


    layout->addWidget(next_match_button);
    layout->addWidget(goto_initial_location_button);
    layout->addWidget(prev_match_button);

    this->setLayout(layout);
}

void SearchButtons::resizeEvent(QResizeEvent* resize_event){

    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    setFixedSize(parent_width, parent_height / 5);
    move(0, 0);
}

ConfigUI::ConfigUI(MainWidget* parent) : QWidget(parent){
    main_widget = parent;
}

void ConfigUI::resizeEvent(QResizeEvent* resize_event){
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    setFixedSize(2 * parent_width / 3, parent_height / 2);
    move(parent_width / 6, parent_height / 4);
}

Color3ConfigUI::Color3ConfigUI(MainWidget* parent, float* config_location_) : ConfigUI(parent) {
    color_location = config_location_;
    color_picker = new QColorDialog(this);
    color_picker->show();

    connect(color_picker, &QColorDialog::colorSelected, [&](const QColor& color){
        convert_qcolor_to_float3(color, color_location);
        main_widget->invalidate_render();
        main_widget->persist_config();
    });
 }

Color4ConfigUI::Color4ConfigUI(MainWidget* parent, float* config_location_) : ConfigUI(parent) {
    color_location = config_location_;
    color_picker = new QColorDialog(this);
    color_picker->show();

    connect(color_picker, &QColorDialog::colorSelected, [&](const QColor& color){
        convert_qcolor_to_float4(color, color_location);
        main_widget->invalidate_render();
        main_widget->persist_config();
    });
 }

FloatConfigUI::FloatConfigUI(MainWidget* parent, float* config_location, float min_value_, float max_value_) : ConfigUI(parent) {

    min_value = min_value_;
    max_value = max_value_;

    float_location = config_location;
    layout = new QHBoxLayout();
    current_value_label = new QLabel(QString::number(*float_location));
    confirm_button = new QPushButton("Confirm", this);
    slider = new QSlider(Qt::Orientation::Horizontal, this);
    slider->setRange(0, 100);
    int current_value = static_cast<int>((*config_location - min_value) / (max_value - min_value) * 100);
    current_value_label->setText(QString::number(*float_location));
    slider->setValue(current_value);

    slider->setStyleSheet("QSlider::groove:horizontal {border: 1px solid;height: 10px;margin: 0px;}QSlider::handle:horizontal {background-color: black;border: 1px solid;height: 40px;width: 40px;margin: -15px 0px;}");

    layout->addWidget(current_value_label);
    layout->addWidget(slider);
    layout->addWidget(confirm_button);


    QObject::connect(confirm_button, &QPushButton::clicked, [&](){
        float value = min_value + (static_cast<float>(slider->value()) / 100) * (max_value - min_value);
        *float_location = value;
        main_widget->invalidate_render();
        main_widget->current_widget = nullptr;
        hide();
        deleteLater();

//        slider->value()
//        *float_location
    });
    QObject::connect(slider, &QSlider::valueChanged, [&](int val){
        float value = min_value + (static_cast<float>(slider->value()) / 100) * (max_value - min_value);
        *float_location = value;
        current_value_label->setText(QString::number(value));
        main_widget->invalidate_render();

    });

    setLayout(layout);
//    slider = new QSlider(this, )
//    color_picker->show();

//    connect(color_picker, &QColorDialog::colorSelected, [&](const QColor& color){
//        convert_qcolor_to_float3(color, color_location);
//        main_widget->invalidate_render();
//    });
 }



BoolConfigUI::BoolConfigUI(MainWidget* parent, bool* config_location, QString name) : ConfigUI(parent) {
    bool_location = config_location;

    layout = new QHBoxLayout();

    label = new QLabel(name, this);
    checkbox = new QCheckBox(this);
    checkbox->setStyleSheet("QCheckBox::indicator {width: 50px;height: 50px;}");

    checkbox->setChecked(*config_location);

    layout->addWidget(label);
    layout->addWidget(checkbox);

    QObject::connect(checkbox, &QCheckBox::stateChanged, [&](int new_state){
        *bool_location = static_cast<bool>(new_state);
        main_widget->invalidate_render();
        main_widget->persist_config();
    });

    setLayout(layout);
}


//QWidget* color3_configurator_ui(MainWidget* main_widget, void* location){
//    return new Color3ConfigUI(main_widget, (float*)location);
//}

//QWidget* color4_configurator_ui(MainWidget* main_widget, void* location){
//    return new Color4ConfigUI(main_widget, (float*)location);
//}

#endif
