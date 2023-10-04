#include "touchui/TouchDrawControls.h"
#include "utils.h"

extern float HIGHLIGHT_COLORS[26 * 3];

TouchDrawControls::TouchDrawControls(float pen_size, char selected_symbol, QWidget* parent) : QWidget(parent) {

    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    QList<QColor> colors;
    const int N_COLORS = 26;
    for (int i = 0; i < N_COLORS; i++) {
        colors.push_back(convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * i]));
    }
    //colors.push_back(convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * ('b' - 'a')]));
    //colors.push_back(convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * ('c' - 'a')]));
    //colors.push_back(convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * ('d' - 'a')]));

    quick_widget->rootContext()->setContextProperty("_colors", QVariant::fromValue(colors));

    quick_widget->rootContext()->setContextProperty("_index", selected_symbol - 'a');
    quick_widget->rootContext()->setContextProperty("_pen_size", pen_size);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchDrawControls.qml"));


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(exitDrawModeButtonClicked()),
        this,
        SLOT(handleExitDrawMode()));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(changeColorClicked(int)),
        this,
        SLOT(handleChangeColor(int)));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(enablePenDrawModeClicked()),
        this,
        SLOT(handleEnablePenDrawMode()));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(disablePenDrawModeClicked()),
        this,
        SLOT(handleDisablePenDrawMode()));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(eraserClicked()),
        this,
        SLOT(handleEraser()));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(penSizeChanged(qreal)),
        this,
        SLOT(handlePenSizeChanged(qreal)));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(screenshotClicked()),
        this,
        SLOT(handleScreenshot()));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(toggleScratchpadClicked()),
        this,
        SLOT(handleToggleScratchpad()));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(saveScratchpadClicked()),
        this,
        SLOT(handleSaveScratchpad()));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(loadScratchpadClicked()),
        this,
        SLOT(handleLoadScratchpad()));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(moveClicked()),
        this,
        SLOT(handleMove()));


}
void TouchDrawControls::setDrawType(char type) {
    quick_widget->rootContext()->setContextProperty("_index", type - 'a');
}

void TouchDrawControls::handleExitDrawMode() {
    emit exitDrawModePressed();
}

void TouchDrawControls::handlePenSizeChanged(qreal val) {
    emit penSizeChanged(val);
}

void TouchDrawControls::handleScreenshot() {
    emit screenshotPressed();
}

void TouchDrawControls::handleEnablePenDrawMode() {
    emit enablePenDrawModePressed();
}

void TouchDrawControls::handleEraser() {
    emit eraserPressed();
}

void TouchDrawControls::handleDisablePenDrawMode() {
    emit disablePenDrawModePressed();
}

void TouchDrawControls::handleToggleScratchpad() {
    emit toggleScratchpadPressed();
}

void TouchDrawControls::handleChangeColor(int index) {
    emit changeColorPressed(index);
}

void TouchDrawControls::handleSaveScratchpad() {
    emit saveScratchpadPressed();
}

void TouchDrawControls::handleLoadScratchpad() {
    emit loadScratchpadPressed();
}

void TouchDrawControls::handleMove() {
    emit movePressed();
}

void TouchDrawControls::resizeEvent(QResizeEvent* resize_event) {
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}

void TouchDrawControls::set_pen_size(float size) {
    quick_widget->rootContext()->setContextProperty("_pen_size", size);
}

void TouchDrawControls::set_scratchpad_mode(bool mode) {
    is_in_scratchpad = mode;

    QMetaObject::invokeMethod(quick_widget->rootObject(), "on_scratchpad_mode_change", QVariant::fromValue(mode));
    //QMetaObject::invokeMethod(quick_widget->rootObject(), "on_scratchpad_mode_change");
}
