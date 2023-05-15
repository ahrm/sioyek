#include "touchui/TouchDrawControls.h"
#include "utils.h"

extern float HIGHLIGHT_COLORS[26 * 3];

TouchDrawControls::TouchDrawControls(char selected_symbol, QWidget* parent) : QWidget(parent){

    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    QList<QColor> colors;
    const int N_COLORS = 5;
    for (int i = 0; i < N_COLORS; i++) {
		colors.push_back(convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * i]));
    }
    //colors.push_back(convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * ('b' - 'a')]));
    //colors.push_back(convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * ('c' - 'a')]));
    //colors.push_back(convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * ('d' - 'a')]));

    quick_widget->rootContext()->setContextProperty("_colors", QVariant::fromValue(colors));

    quick_widget->rootContext()->setContextProperty("_index", selected_symbol - 'a');

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


}
void TouchDrawControls::setDrawType(char type) {
    quick_widget->rootContext()->setContextProperty("_index", type - 'a');
}

void TouchDrawControls::handleExitDrawMode() {
    emit exitDrawModePressed();
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

void TouchDrawControls::handleChangeColor(int index) {
    emit changeColorPressed(index);
}

void TouchDrawControls::resizeEvent(QResizeEvent* resize_event){
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}
