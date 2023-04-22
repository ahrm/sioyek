#include "touchui/TouchSettings.h"
#include "touchui/TouchConfigMenu.h"
#include "main_widget.h"


extern float BACKGROUND_COLOR[3];
extern float DARK_MODE_BACKGROUND_COLOR[3];
extern float CUSTOM_COLOR_MODE_EMPTY_BACKGROUND_COLOR[3];
extern float CUSTOM_BACKGROUND_COLOR[3];
extern float CUSTOM_TEXT_COLOR[3];
extern float VISUAL_MARK_NEXT_PAGE_FRACTION;
extern float VISUAL_MARK_NEXT_PAGE_THRESHOLD;
extern UIRect PORTRAIT_VISUAL_MARK_PREV;
extern UIRect PORTRAIT_VISUAL_MARK_NEXT;
extern UIRect LANDSCAPE_VISUAL_MARK_PREV;
extern UIRect LANDSCAPE_VISUAL_MARK_NEXT;
extern UIRect PORTRAIT_BACK_UI_RECT;
extern UIRect PORTRAIT_FORWARD_UI_RECT;
extern UIRect LANDSCAPE_BACK_UI_RECT;
extern UIRect LANDSCAPE_FORWARD_UI_RECT;
extern UIRect PORTRAIT_EDIT_PORTAL_UI_RECT;
extern UIRect LANDSCAPE_EDIT_PORTAL_UI_RECT;
extern float DEFAULT_VERTICAL_LINE_COLOR[4];

TouchSettings::TouchSettings(MainWidget* parent) : QWidget(parent){

    setAttribute(Qt::WA_NoMousePropagation);

    main_widget = parent;
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    //quick_widget->rootContext()->setContextProperty("_currentColorschemeIndex", current_colorscheme_index);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchSettings.qml"));


    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(lightApplicationBackgroundClicked()),
                this,
                SLOT(handleLightApplicationBackground()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(darkApplicationBackgroundClicked()),
                this,
                SLOT(handleDarkApplicationBackground()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(customApplicationBackgroundClicked()),
                this,
                SLOT(handleCustomApplicationBackground()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(customPageTextClicked()),
                this,
                SLOT(handleCustomPageText()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(customPageBackgroundClicked()),
                this,
                SLOT(handleCustomPageBackground()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(rulerModeBoundsClicked()),
                this,
                SLOT(handleRulerModeBounds()));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(rulerModeColorClicked()),
                this,
                SLOT(handleRulerModeColor())); 

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(rulerNextClicked()),
                this,
                SLOT(handleRulerNext())); 

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(rulerPrevClicked()),
                this,
                SLOT(handleRulerPrev())); 

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(portalClicked()),
                this,
                SLOT(handlePortal())); 
    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(backClicked()),
                this,
                SLOT(handleBack())); 

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(forwardClicked()),
                this,
                SLOT(handleForward())); 

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(allConfigsClicked()),
                this,
                SLOT(handleAllConfigs())); 

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(restoreDefaultsClicked()),
                this,
                SLOT(handleRestoreDefaults())); 

}


void TouchSettings::handleLightApplicationBackground(){
    show_dialog_for_color_n(3, BACKGROUND_COLOR);
}

void TouchSettings::handleDarkApplicationBackground() {
    show_dialog_for_color_n(3, DARK_MODE_BACKGROUND_COLOR);
}

void TouchSettings::handleCustomApplicationBackground() {
    show_dialog_for_color_n(3, CUSTOM_COLOR_MODE_EMPTY_BACKGROUND_COLOR);
}

void TouchSettings::handleCustomPageText() {
    show_dialog_for_color_n(3, CUSTOM_TEXT_COLOR);
}

void TouchSettings::handleCustomPageBackground() {
    show_dialog_for_color_n(3, CUSTOM_BACKGROUND_COLOR);
}



void TouchSettings::resizeEvent(QResizeEvent* resize_event){
    //quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    //QWidget::resizeEvent(resize_event);

    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    //float parent_width_in_centimeters = static_cast<float>(parent_width) / logicalDpiX() * 2.54f;
    //float parent_height_in_centimeters = static_cast<float>(parent_height) / logicalDpiY() * 2.54f;
    int ten_cm = static_cast<int>(12 * logicalDpiX() / 2.54f );

    int w = static_cast<int>(parent_width * 0.9f);
    int h = parent_height;

    w = std::min(w, ten_cm);
    h = std::min(h, static_cast<int>(ten_cm * 1.5f));

    quick_widget->resize(w, h);
    setFixedSize(w, h);

//    list_view->setFixedSize(parent_width * 0.9f, parent_height);
    move((parent_width - w) / 2, (parent_height - h) / 2);

}
void TouchSettings::show_dialog_for_color_n(int n, float* color_location) {
    QColorDialog* dialog = new QColorDialog(this);

    if (n == 4) {
		dialog->setOption(QColorDialog::ShowAlphaChannel, true);
    }

    main_widget->push_current_widget(dialog);
    dialog->show();

    connect(dialog, &QColorDialog::colorSelected, [&, color_location, n](const QColor& color){
        if (n == 4) {
			convert_qcolor_to_float4(color, color_location);
        }
        else {
			convert_qcolor_to_float3(color, color_location);
        }
        main_widget->invalidate_render();
        main_widget->persist_config();
        //main_widget->pop_current_widget();
    });

    connect(dialog, &QColorDialog::finished, [&](){
        main_widget->pop_current_widget();
    });

}

void TouchSettings::handleRulerModeBounds() {
    RangeConfigUI *config_ui = new RangeConfigUI(main_widget, &VISUAL_MARK_NEXT_PAGE_FRACTION, &VISUAL_MARK_NEXT_PAGE_THRESHOLD);
    config_ui->set_should_persist(true);
    config_ui->show();
    main_widget->push_current_widget(config_ui);
}

void TouchSettings::handleRulerModeColor() {
    show_dialog_for_color_n(4, DEFAULT_VERTICAL_LINE_COLOR);
}

void TouchSettings::show_dialog_for_rect(UIRect* location) {
    RectangleConfigUI* config_ui = new RectangleConfigUI(main_widget, location);
    config_ui->set_should_persist(true);
    config_ui->show();
    main_widget->push_current_widget(config_ui);
}

void TouchSettings::handleRulerNext() {
    if (screen()->orientation() == Qt::PortraitOrientation) {
        show_dialog_for_rect(&PORTRAIT_VISUAL_MARK_NEXT);
    }
    else{
        show_dialog_for_rect(&LANDSCAPE_VISUAL_MARK_NEXT);
    }
}

void TouchSettings::handleRulerPrev() {
    if (screen()->orientation() == Qt::PortraitOrientation) {
        show_dialog_for_rect(&PORTRAIT_VISUAL_MARK_PREV);
    }
    else{
        show_dialog_for_rect(&LANDSCAPE_VISUAL_MARK_PREV);
    }
}

void TouchSettings::handleBack() {
    if (screen()->orientation() == Qt::PortraitOrientation) {
        show_dialog_for_rect(&PORTRAIT_BACK_UI_RECT);
    }
    else{
        show_dialog_for_rect(&LANDSCAPE_BACK_UI_RECT);
    }
}

void TouchSettings::handlePortal() {
    if (screen()->orientation() == Qt::PortraitOrientation) {
        show_dialog_for_rect(&PORTRAIT_EDIT_PORTAL_UI_RECT);
    }
    else{
        show_dialog_for_rect(&LANDSCAPE_EDIT_PORTAL_UI_RECT);
    }
}

void TouchSettings::handleForward() {
    if (screen()->orientation() == Qt::PortraitOrientation) {
        show_dialog_for_rect(&PORTRAIT_FORWARD_UI_RECT);
    }
    else{
        show_dialog_for_rect(&LANDSCAPE_FORWARD_UI_RECT);
    }
}

void TouchSettings::handleAllConfigs() {
	TouchConfigMenu* config_menu = new TouchConfigMenu(main_widget);
	main_widget->push_current_widget(config_menu);
	main_widget->show_current_widget();
}

void TouchSettings::handleRestoreDefaults() {
    main_widget->restore_default_config();
}

