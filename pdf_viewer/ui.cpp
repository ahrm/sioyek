#include "ui.h"
#include <qfiledialog.h>

#include <QItemSelectionModel>
#include <QTapGesture>
#include <main_widget.h>

#include <QQuickView>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQuickWidget>
#include <QFile>
#include "touchui/TouchSlider.h"

extern std::wstring DEFAULT_OPEN_FILE_PATH;
extern float DARK_MODE_CONTRAST;
extern float BACKGROUND_COLOR[3];
extern bool RULER_MODE;
extern float VISUAL_MARK_NEXT_PAGE_FRACTION;
extern float VISUAL_MARK_NEXT_PAGE_THRESHOLD;

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

 AndroidSelector::AndroidSelector(QWidget* parent) : QWidget(parent){
//     layout = new QVBoxLayout();

     main_widget = dynamic_cast<MainWidget*>(parent);
     int current_colorscheme_index = main_widget->get_current_colorscheme_index();
     main_menu = new TouchMainMenu(current_colorscheme_index, this);


//    set_rect_config_button = new QPushButton("Rect Config", this);
//    goto_page_button = new QPushButton("Goto Page", this);
//    fullscreen_button = new QPushButton("Fullscreen", this);
//    select_text_button = new QPushButton("Select Text", this);
//    open_document_button = new QPushButton("Open New Document", this);
//    open_prev_document_button = new QPushButton("Open Previous Document", this);
//    command_button = new QPushButton("Command", this);
//    visual_mode_button = new QPushButton("Visual Mark Mode", this);
//    search_button = new QPushButton("Search", this);
//    set_background_color = new QPushButton("Background Color", this);
//    set_dark_mode_contrast = new QPushButton("Dark Mode Contrast", this);
//    set_ruler_mode = new QPushButton("Ruler Mode", this);
//    restore_default_config_button = new QPushButton("Restore Default Config", this);
//    toggle_dark_mode_button = new QPushButton("Toggle Dark Mode", this);
//    ruler_mode_bounds_config_button = new QPushButton("Configure Ruler Mode", this);
//    test_rectangle_select_ui = new QPushButton("Rectangle Select", this);

//    layout->addWidget(set_rect_config_button);
//    layout->addWidget(goto_page_button);
//    layout->addWidget(fullscreen_button);
//    layout->addWidget(select_text_button);
//    layout->addWidget(open_document_button);
//    layout->addWidget(open_prev_document_button);
//    layout->addWidget(command_button);
//    layout->addWidget(visual_mode_button);
//    layout->addWidget(search_button);
//    layout->addWidget(set_background_color);
//    layout->addWidget(set_dark_mode_contrast);
//    layout->addWidget(set_ruler_mode);
//    layout->addWidget(restore_default_config_button);
//    layout->addWidget(toggle_dark_mode_button);
//    layout->addWidget(ruler_mode_bounds_config_button);
//    layout->addWidget(test_rectangle_select_ui);


    QObject::connect(main_menu, &TouchMainMenu::fullscreenClicked, [&](){
        main_widget->current_widget = {};
        deleteLater();
        main_widget->toggle_fullscreen();
    });

    QObject::connect(main_menu, &TouchMainMenu::selectTextClicked, [&](){
        main_widget->current_widget = {};
        deleteLater();
        main_widget->handle_mobile_selection();
        main_widget->invalidate_render();
    });

    QObject::connect(main_menu, &TouchMainMenu::openNewDocClicked, [&](){
        main_widget->current_widget = {};
        deleteLater();
        auto command = main_widget->command_manager->get_command_with_name("open_document");
        main_widget->handle_command_types(std::move(command), 0);
    });

    QObject::connect(main_menu, &TouchMainMenu::openPrevDocClicked, [&](){
        auto command = main_widget->command_manager->get_command_with_name("open_prev_doc");
        main_widget->current_widget = {};
        deleteLater();
        main_widget->handle_command_types(std::move(command), 0);
    });

    QObject::connect(main_menu, &TouchMainMenu::commandsClicked, [&](){
        auto command = main_widget->command_manager->get_command_with_name("command");
        main_widget->current_widget = {};
        deleteLater();
        main_widget->handle_command_types(std::move(command), 0);
    });

    QObject::connect(main_menu, &TouchMainMenu::rulerModeClicked, [&](){
        main_widget->android_handle_visual_mode();
        main_widget->current_widget = {};
        deleteLater();
        main_widget->invalidate_render();
    });

    QObject::connect(main_menu, &TouchMainMenu::searchClicked, [&](){
        auto command = main_widget->command_manager->get_command_with_name("search");
        main_widget->current_widget = {};
        deleteLater();
        main_widget->handle_command_types(std::move(command), 0);
    });

//    QObject::connect(set_background_color, &QPushButton::pressed, [&](){

//        auto command = main_widget->command_manager->get_command_with_name("setconfig_background_color");
//        main_widget->current_widget = {};
//        deleteLater();
//        main_widget->handle_command_types(std::move(command), 0);
//    });

//    QObject::connect(set_rect_config_button, &QPushButton::pressed, [&](){

//        auto command = main_widget->command_manager->get_command_with_name("setconfig_portrait_back_ui_rect");
//        main_widget->current_widget = {};
//        deleteLater();
//        main_widget->handle_command_types(std::move(command), 0);
//    });

//    QObject::connect(set_dark_mode_contrast, &QPushButton::pressed, [&](){

//        main_widget->current_widget = {};
//        deleteLater();

//        main_widget->current_widget = new FloatConfigUI(main_widget, &DARK_MODE_CONTRAST, 0.0f, 1.0f);
//        main_widget->current_widget->show();
//    });

//    QObject::connect(set_ruler_mode, &QPushButton::pressed, [&](){

//        auto command = main_widget->command_manager->get_command_with_name("setconfig_ruler_mode");
//        main_widget->current_widget = {};
//        deleteLater();
//        main_widget->handle_command_types(std::move(command), 0);

//    });

//    QObject::connect(restore_default_config_button, &QPushButton::pressed, [&](){

//        main_widget->current_widget = {};
//        deleteLater();
//        main_widget->restore_default_config();

//    });

    QObject::connect(main_menu, &TouchMainMenu::darkColorschemeClicked, [&](){
        main_widget->set_dark_mode();
        main_widget->current_widget = {};
        deleteLater();
        main_widget->invalidate_render();
    });

    QObject::connect(main_menu, &TouchMainMenu::lightColorschemeClicked, [&](){
        main_widget->set_light_mode();
        main_widget->current_widget = {};
        deleteLater();
        main_widget->invalidate_render();
    });

    QObject::connect(main_menu, &TouchMainMenu::customColorschemeClicked, [&](){
        main_widget->set_custom_color_mode();
        main_widget->current_widget = {};
        deleteLater();
        main_widget->invalidate_render();
    });

//    QObject::connect(ruler_mode_bounds_config_button, &QPushButton::pressed, [&](){
////        auto command = main_widget->command_manager->get_command_with_name("toggle_dark_mode");
//        main_widget->current_widget = nullptr;
//        deleteLater();
//        RangeConfigUI* config_ui = new RangeConfigUI(main_widget, &VISUAL_MARK_NEXT_PAGE_FRACTION, &VISUAL_MARK_NEXT_PAGE_THRESHOLD);
//        main_widget->current_widget = config_ui;
//        main_widget->current_widget->show();
////        main_widget->handle_command_types(std::move(command), 0);
//    });

    QObject::connect(main_menu, &TouchMainMenu::gotoPageClicked, [&](){
        main_widget->current_widget = nullptr;
        deleteLater();
        PageSelectorUI* page_ui = new PageSelectorUI(main_widget,
                                                     main_widget->main_document_view->get_center_page_number(),
                                                     main_widget->doc()->num_pages());

        main_widget->current_widget = page_ui;
        main_widget->current_widget->show();
    });

//    QObject::connect(test_rectangle_select_ui, &QPushButton::pressed, [&](){
////        auto command = main_widget->command_manager->get_command_with_name("toggle_dark_mode");
////        main_widget->current_widget = {};
//        RectangleConfigUI* configui = new RectangleConfigUI(main_widget, &testrect);
//        main_widget->current_widget = configui;
//        configui->show();

//        deleteLater();
////        main_widget->handle_command_types(std::move(command), 0);
//    });

//     layout->insertStretch(-1, 1);

//     this->setLayout(layout);
 }

void AndroidSelector::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();


    int w = static_cast<int>(parent_width * 0.9f);
    int h = parent_height;

    main_menu->resize(w, h);
    setFixedSize(w, h);

//    list_view->setFixedSize(parent_width * 0.9f, parent_height);
    move(parent_width * 0.05f, 0);

}

//TextSelectionButtons::TextSelectionButtons(MainWidget* parent) : QWidget(parent) {

//    QHBoxLayout* layout = new QHBoxLayout();

//    main_widget = parent;
//    copy_button = new QPushButton("Copy");
//    search_in_scholar_button = new QPushButton("Search Scholar");
//    search_in_google_button = new QPushButton("Search Google");
//    highlight_button = new QPushButton("Highlight");

//    QObject::connect(copy_button, &QPushButton::clicked, [&](){
//        copy_to_clipboard(main_widget->selected_text);
//    });

//    QObject::connect(search_in_scholar_button, &QPushButton::clicked, [&](){
//        search_custom_engine(main_widget->selected_text, L"https://scholar.google.com/scholar?&q=");
//    });

//    QObject::connect(search_in_google_button, &QPushButton::clicked, [&](){
//        search_custom_engine(main_widget->selected_text, L"https://www.google.com/search?q=");
//    });

//    QObject::connect(highlight_button, &QPushButton::clicked, [&](){
//        main_widget->handle_touch_highlight();
//    });

//    layout->addWidget(copy_button);
//    layout->addWidget(search_in_scholar_button);
//    layout->addWidget(search_in_google_button);
//    layout->addWidget(highlight_button);

//    this->setLayout(layout);
//}

//void TextSelectionButtons::resizeEvent(QResizeEvent* resize_event) {
//    QWidget::resizeEvent(resize_event);
//    int parent_width = parentWidget()->width();
//    int parent_height = parentWidget()->height();

//    setFixedSize(parent_width, parent_height / 5);
////    list_view->setFixedSize(parent_width * 0.9f, parent_height);
//    move(0, 0);

//}

TouchTextSelectionButtons::TouchTextSelectionButtons(MainWidget* parent) : QWidget(parent){
    main_widget = parent;
    buttons_ui = new TouchCopyOptions(this);
    this->setAttribute(Qt::WA_NoMousePropagation);

    QObject::connect(buttons_ui, &TouchCopyOptions::copyClicked, [&](){
        copy_to_clipboard(main_widget->selected_text);
    });

    QObject::connect(buttons_ui, &TouchCopyOptions::scholarClicked, [&](){
        search_custom_engine(main_widget->selected_text, L"https://scholar.google.com/scholar?&q=");
    });

    QObject::connect(buttons_ui, &TouchCopyOptions::googleClicked, [&](){
        search_custom_engine(main_widget->selected_text, L"https://www.google.com/search?q=");
    });

    QObject::connect(buttons_ui, &TouchCopyOptions::highlightClicked, [&](){
        main_widget->handle_touch_highlight();
    });
}

void TouchTextSelectionButtons::resizeEvent(QResizeEvent* resize_event){
    QWidget::resizeEvent(resize_event);
//    qDebug() << "sioyek: resize " << resize_event << "\n";

//    int width = resize_event->size().width() * 2 / 3;
//    int height = resize_event->size().height() / 4;

    int pwidth = parentWidget()->width();
    int width = parentWidget()->width() * 3 / 4;
    int height = parentWidget()->height() / 16;

    buttons_ui->move(0, 0);
    buttons_ui->resize(width, height);
    move((pwidth - width) / 2, height);
//    setFixedSize(0, 0);
    setFixedSize(width, height);

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

//ConfigUI::ConfigUI(MainWidget* parent) : QQuickWidget(parent){
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

//    QQmlEngine* engine = new QQmlEngine();
//    QQmlComponent* component = new QQmlComponent(engine, QUrl("qrc:/pdf_viewer/qml/MyColorPicker.qml"), this);
//    QObject *object = component->create();
//    QQuickItem *item = qobject_cast<QQuickItem*>(object);

//    QFile source_file("qrc:/pdf_viewer/qml/MyColorPicker.qml");
//    QString source = source_file.readAll();

//    QQuickWidget* quick_widget(source, this);
//    QUrl url("qrc:/pdf_viewer/qml/MyColorPicker.qml");
//    color_picker = new QQuickWidget(url, this);
//    color_picker->setSizePolicy(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Maximum);
//    color_picker->show();
//    TouchSlider* slider = new TouchSlider(0, 100, 25, this);
//    QObject::connect(slider, &TouchSlider::itemSelected, [&](int selected_value){
//        qDebug() << "selected " << selected_value << "\n";

//    });


//    QObject::connect(color_picker, SIGNAL()

//    QQuickWidget
//    QQuickView* view = new QQuickView();
//    view->setSource(QUrl("qrc:/pdf_viewer/qml/MyColorPicker.qml"));
//    view->show();


 }

void Color3ConfigUI::resizeEvent(QResizeEvent* resize_event){
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    setFixedSize(2 * parent_width / 3, parent_height / 2);
    color_picker->resize(width(), height());

    move(parent_width / 6, parent_height / 4);
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

    int current_value = static_cast<int>((*config_location - min_value) / (max_value - min_value) * 100);
    slider = new TouchSlider(0, 100, current_value, this);
    QObject::connect(slider, &TouchSlider::itemSelected, [&](int val){
        float value = min_value + (static_cast<float>(val) / 100.0f) * (max_value - min_value);
        *float_location = value;
        main_widget->invalidate_render();
        main_widget->current_widget = nullptr;
        deleteLater();
    });

//    layout = new QHBoxLayout();
//    current_value_label = new QLabel(QString::number(*float_location));
//    confirm_button = new QPushButton("Confirm", this);
//    slider = new QSlider(Qt::Orientation::Horizontal, this);
//    slider->setRange(0, 100);
//    current_value_label->setText(QString::number(*float_location));
//    slider->setValue(current_value);

//    slider->setStyleSheet("QSlider::groove:horizontal {border: 1px solid;height: 10px;margin: 0px;}QSlider::handle:horizontal {background-color: black;border: 1px solid;height: 40px;width: 40px;margin: -15px 0px;}");

//    layout->addWidget(current_value_label);
//    layout->addWidget(slider);
//    layout->addWidget(confirm_button);


//    QObject::connect(confirm_button, &QPushButton::clicked, [&](){
//        float value = min_value + (static_cast<float>(slider->value()) / 100) * (max_value - min_value);
//        *float_location = value;
//        main_widget->invalidate_render();
//        main_widget->current_widget = nullptr;
//        hide();
//        deleteLater();

////        slider->value()
////        *float_location
//    });
//    QObject::connect(slider, &QSlider::valueChanged, [&](int val){
//        float value = min_value + (static_cast<float>(slider->value()) / 100) * (max_value - min_value);
//        *float_location = value;
//        current_value_label->setText(QString::number(value));
//        main_widget->invalidate_render();

//    });

//    setLayout(layout);
//    slider = new QSlider(this, )
//    color_picker->show();

//    connect(color_picker, &QColorDialog::colorSelected, [&](const QColor& color){
//        convert_qcolor_to_float3(color, color_location);
//        main_widget->invalidate_render();
//    });
 }

PageSelectorUI::PageSelectorUI(MainWidget* parent, int current, int num_pages) : ConfigUI(parent) {

     page_selector = new TouchPageSelector(0, num_pages-1, current, this);

    QObject::connect(page_selector, &TouchPageSelector::pageSelected, [&](int val){
        main_widget->main_document_view->goto_page(val);
        main_widget->invalidate_render();
    });

 }

void PageSelectorUI::resizeEvent(QResizeEvent* resize_event){
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int w = 2 * parent_width / 3;
    int h =  parent_height / 2;
    page_selector->resize(w, h);

    setFixedSize(w, h);
    move(parent_width / 6, parent_height / 4);
}



BoolConfigUI::BoolConfigUI(MainWidget* parent, bool* config_location, QString name) : ConfigUI(parent) {
    bool_location = config_location;

    checkbox = new TouchCheckbox(name, *config_location, this);
    QObject::connect(checkbox, &TouchCheckbox::itemSelected, [&](bool new_state){
        *bool_location = static_cast<bool>(new_state);
        main_widget->invalidate_render();
        main_widget->persist_config();

        main_widget->current_widget = nullptr;
        deleteLater();
    });
//    layout = new QHBoxLayout();

//    label = new QLabel(name, this);
//    checkbox = new QCheckBox(this);
//    checkbox->setStyleSheet("QCheckBox::indicator {width: 50px;height: 50px;}");

//    checkbox->setChecked(*config_location);

//    layout->addWidget(label);
//    layout->addWidget(checkbox);

//    QObject::connect(checkbox, &QCheckBox::stateChanged, [&](int new_state){
//        *bool_location = static_cast<bool>(new_state);
//        main_widget->invalidate_render();
//        main_widget->persist_config();
//    });

//    setLayout(layout);
}

void BoolConfigUI::resizeEvent(QResizeEvent* resize_event){
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int w = 2 * parent_width / 3;
    int h =  parent_height / 2;
    checkbox->resize(w, h);

    setFixedSize(w, h);
    move(parent_width / 6, parent_height / 4);
}

void FloatConfigUI::resizeEvent(QResizeEvent* resize_event){
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int w = 2 * parent_width / 3;
    int h =  parent_height / 2;
    slider->resize(w, h);

    setFixedSize(w, h);
    move(parent_width / 6, parent_height / 4);
}

//QWidget* color3_configurator_ui(MainWidget* main_widget, void* location){
//    return new Color3ConfigUI(main_widget, (float*)location);
//}

//QWidget* color4_configurator_ui(MainWidget* main_widget, void* location){
//    return new Color4ConfigUI(main_widget, (float*)location);
//}


TouchCommandSelector::TouchCommandSelector(const QStringList& commands, MainWidget* mw): QWidget(mw){
    main_widget = mw;
    list_view = new TouchListView(commands, this);

    QObject::connect(list_view, &TouchListView::itemSelected, [&](QString val, int index){
        main_widget->current_widget = nullptr;
        main_widget->on_command_done(val.toStdString());
        deleteLater();
    });
}

void TouchCommandSelector::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    qDebug() << "sioyek :" << resize_event << "\n";

    int parent_width = parentWidget()->size().width();
    int parent_height = parentWidget()->size().height();

    resize(parent_width * 0.9f, parent_height);
    move(parent_width * 0.05f, 0);
    list_view->resize(parent_width * 0.9f, parent_height);
}


RectangleConfigUI::RectangleConfigUI(MainWidget* parent, UIRect* config_location) : ConfigUI(parent) {

    rect_location = config_location;

    bool current_enabled = config_location->enabled;

    float current_left = config_location->left;
    float current_right = config_location->right;
    float current_top = config_location->top;
    float current_bottom = config_location->bottom;

//    layout = new QVBoxLayout();

    rectangle_select_ui = new TouchRectangleSelectUI(current_enabled,
                                                     current_left,
                                                     current_top,
                                                     (current_right - current_left) / 2.0f,
                                                     (current_bottom - current_top) / 2.0f,
                                                     this);


//    int current_value = static_cast<int>((*config_location - min_value) / (max_value - min_value) * 100);
//    slider = new TouchSlider(0, 100, current_value, this);
    QObject::connect(rectangle_select_ui, &TouchRectangleSelectUI::rectangleSelected, [&](bool enabled, qreal left, qreal right, qreal top, qreal bottom){

        rect_location->enabled = enabled;
        rect_location->left = left;
        rect_location->right = right;
        rect_location->top = top;
        rect_location->bottom = bottom;

        main_widget->persist_config();
        main_widget->invalidate_render();
        main_widget->current_widget = nullptr;
        deleteLater();
    });

 }

//bool RectangleConfigUI::eventFilter(QObject *obj, QEvent *event){
//    if ((obj == parentWidget()) && (event->type() == QEvent::Type::Resize)){

//        QResizeEvent* resize_event = static_cast<QResizeEvent*>(event);
//        int parent_width = resize_event->size().width();
//        int parent_height = resize_event->size().height();

//        bool res = QObject::eventFilter(obj, event);

//        qDebug() << "resizing to " << parent_width << " " << parent_height;
//        rectangle_select_ui->resize(parent_width, parent_height);
//        setFixedSize(parent_width, parent_height);

////        setFixedSize(parent_width, parent_height);
//        move(0, 0);
//        return res;
//    }
//    else{
//        return QObject::eventFilter(obj, event);
//    }
//}

void RectangleConfigUI::resizeEvent(QResizeEvent* resize_event){
    QWidget::resizeEvent(resize_event);
    move(0, 0);
//    rectangle_select_ui->resize(resize_event->size().width(), resize_event->size().height());
    setFixedSize(parentWidget()->size());
    rectangle_select_ui->resize(parentWidget()->size());

}

RangeConfigUI::RangeConfigUI(MainWidget* parent, float* top_config_location, float* bottom_config_location) : ConfigUI(parent) {

//    range_location = config_location;
    top_location = top_config_location;
    bottom_location = bottom_config_location;

    float current_top = -(*top_location);
    float current_bottom = -(*bottom_location) + 1;


    range_select_ui = new TouchRangeSelectUI(current_top, current_bottom, this);

    // force a resize event in order to have correct sizes
    resize(parent->width(), parent->height());


    QObject::connect(range_select_ui, &TouchRangeSelectUI::rangeSelected, [&](qreal top, qreal bottom){

        *top_location = -top;
        *bottom_location = -bottom + 1;

        main_widget->persist_config();
        main_widget->invalidate_render();
        main_widget->current_widget = nullptr;
        deleteLater();
    });

    QObject::connect(range_select_ui, &TouchRangeSelectUI::rangeCanceled, [&](){
        main_widget->invalidate_render();
        main_widget->current_widget = nullptr;
        deleteLater();
    });

 }

void RangeConfigUI::resizeEvent(QResizeEvent* resize_event){
    QWidget::resizeEvent(resize_event);
    move(0, 0);
    range_select_ui->resize(resize_event->size().width(), resize_event->size().height());

}
