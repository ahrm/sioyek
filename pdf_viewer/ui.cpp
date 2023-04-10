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
#include "touchui/TouchConfigMenu.h"

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
        //main_widget->current_widget = {};
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        //deleteLater();
        main_widget->toggle_fullscreen();
    });

    QObject::connect(main_menu, &TouchMainMenu::selectTextClicked, [&](){
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        main_widget->handle_mobile_selection();
        main_widget->invalidate_render();
    });

    QObject::connect(main_menu, &TouchMainMenu::openNewDocClicked, [&](){
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        auto command = main_widget->command_manager->get_command_with_name("open_document");
        main_widget->handle_command_types(std::move(command), 0);
    });

    QObject::connect(main_menu, &TouchMainMenu::openPrevDocClicked, [&](){
        auto command = main_widget->command_manager->get_command_with_name("open_prev_doc");
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        main_widget->handle_command_types(std::move(command), 0);
    });

    QObject::connect(main_menu, &TouchMainMenu::commandsClicked, [&](){
        auto command = main_widget->command_manager->get_command_with_name("command");
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        main_widget->handle_command_types(std::move(command), 0);
    });

    QObject::connect(main_menu, &TouchMainMenu::settingsClicked, [&](){
        TouchConfigMenu* config_menu = new TouchConfigMenu(main_widget);
        //main_widget->current_widget = new TouchConfigMenu(main_widget);
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();

        main_widget->set_current_widget(config_menu);
        main_widget->show_current_widget();

        //auto command = main_widget->command_manager->get_command_with_name("command");
        //main_widget->current_widget = {};
        //deleteLater();
        //main_widget->handle_command_types(std::move(command), 0);
    });

    QObject::connect(main_menu, &TouchMainMenu::rulerModeClicked, [&](){
        main_widget->android_handle_visual_mode();
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        main_widget->invalidate_render();
    });

    QObject::connect(main_menu, &TouchMainMenu::searchClicked, [&](){
        auto command = main_widget->command_manager->get_command_with_name("search");
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
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
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        main_widget->invalidate_render();
    });

    QObject::connect(main_menu, &TouchMainMenu::lightColorschemeClicked, [&](){
        main_widget->set_light_mode();
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        main_widget->invalidate_render();
    });

    QObject::connect(main_menu, &TouchMainMenu::customColorschemeClicked, [&](){
        main_widget->set_custom_color_mode();
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
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
        //main_widget->current_widget = nullptr;
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        PageSelectorUI* page_ui = new PageSelectorUI(main_widget,
                                                     main_widget->main_document_view->get_center_page_number(),
                                                     main_widget->doc()->num_pages());

        main_widget->set_current_widget(page_ui);
        main_widget->show_current_widget();
        //main_widget->current_widget = page_ui;
        //main_widget->current_widget->show();
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
        main_widget->pop_current_widget();
    });

 }


IntConfigUI::IntConfigUI(MainWidget* parent, int* config_location, int min_value_, int max_value_) : ConfigUI(parent) {

    min_value = min_value_;
    max_value = max_value_;
    int_location = config_location;

    int current_value = static_cast<int>((*config_location - min_value) / (max_value - min_value) * 100);
    slider = new TouchSlider(min_value, max_value, current_value, this);
    QObject::connect(slider, &TouchSlider::itemSelected, [&](int val){
        *int_location = val;
        main_widget->invalidate_render();
        main_widget->pop_current_widget();
    });

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

        //main_widget->current_widget = nullptr;
        //deleteLater();
        main_widget->pop_current_widget();
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

void IntConfigUI::resizeEvent(QResizeEvent* resize_event){
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
        //main_widget->current_widget = nullptr;
        main_widget->pop_current_widget();
        main_widget->on_command_done(val.toStdString());
        //deleteLater();
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
        //main_widget->current_widget = nullptr;
        //deleteLater();
        main_widget->pop_current_widget();
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
        //main_widget->current_widget = nullptr;
        //deleteLater();
        main_widget->pop_current_widget();
    });

    QObject::connect(range_select_ui, &TouchRangeSelectUI::rangeCanceled, [&](){
        main_widget->invalidate_render();
        //main_widget->current_widget = nullptr;
        //deleteLater();
        main_widget->pop_current_widget();
    });

 }

void RangeConfigUI::resizeEvent(QResizeEvent* resize_event){
    QWidget::resizeEvent(resize_event);
    move(0, 0);
    range_select_ui->resize(resize_event->size().width(), resize_event->size().height());

}
QList<QStandardItem*> CommandSelector::get_item(std::string command_name) {

	std::string command_key = "";

	if (key_map.find(command_name) != key_map.end()) {
		const std::vector<std::string>& command_keys = key_map[command_name];
		for (size_t i = 0; i < command_keys.size(); i++) {
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

QStandardItemModel* CommandSelector::get_standard_item_model(std::vector<std::string> command_names) {

	QStandardItemModel* res = new QStandardItemModel();

	for (size_t i = 0; i < command_names.size(); i++) {
		res->appendRow(get_item(command_names[i]));
	}
	return res;
}

QStandardItemModel* CommandSelector::get_standard_item_model(QStringList command_names) {

	std::vector<std::string> std_command_names;

	for (int i = 0; i < command_names.size(); i++) {
		std_command_names.push_back(command_names.at(i).toStdString());
	}
	return get_standard_item_model(std_command_names);
}


QString CommandSelector::get_view_stylesheet_type_name() {
	return "QTableView";
}

void CommandSelector::on_select(const QModelIndex& index) {
	bool is_numeric = false;
	line_edit->text().toInt(&is_numeric);
	QString name = standard_item_model->data(index).toString();
	//hide();
    main_widget->pop_current_widget();
	parentWidget()->setFocus();
	if (!is_numeric) {
		(*on_done)(name.toStdString());
	}
	else {
		(*on_done)(line_edit->text().toStdString());
	}
}

CommandSelector::CommandSelector(std::function<void(std::string)>* on_done,
	MainWidget* parent,
	QStringList elements,
	std::unordered_map<std::string,
	std::vector<std::string>> key_map) : BaseSelectorWidget<std::string, QTableView, MySortFilterProxyModel>(nullptr, parent),
	key_map(key_map),
	on_done(on_done),
    main_widget(parent)
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

bool CommandSelector::on_text_change(const QString& text) {

	std::vector<std::string> matching_element_names;
	std::vector<int> scores;
	std::string search_text_string = text.toStdString();
	std::vector<std::pair<std::string, int>> match_score_pairs;

	for (int i = 0; i < string_elements.size(); i++) {
		std::string encoded = utf8_encode(string_elements.at(i).toStdWString());
		int score = 0;
		if (FUZZY_SEARCHING) {
			score = static_cast<int>(rapidfuzz::fuzz::partial_ratio(search_text_string, encoded));
		}
		else {
			fts::fuzzy_match(search_text_string.c_str(), encoded.c_str(), score);
		}
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

	//if (matching_element_names.size() == 0) {
	for (auto [command, score] : match_score_pairs) {
		if (score > 60 && (!QString::fromStdString(command).startsWith(text))) {
			matching_element_names.push_back(command);
		}
	}
	//}

	QStandardItemModel* new_standard_item_model = get_standard_item_model(matching_element_names);
	dynamic_cast<QTableView*>(get_view())->setModel(new_standard_item_model);
	delete standard_item_model;
	standard_item_model = new_standard_item_model;
	return true;
}

