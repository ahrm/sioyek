#include "touchui/TouchHighlightButtons.h"
#include "utils.h"

extern float HIGHLIGHT_COLORS[26 * 3];

TouchHighlightButtons::TouchHighlightButtons(char selected_symbol, QWidget* parent) : QWidget(parent){

//    quick_widget = new QQuickWidget(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"), this);
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    //quick_widget->rootContext()->setContextProperty("_initialValue", initial_value);
    quick_widget->rootContext()->setContextProperty("_color_a", QVariant::fromValue(convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * ('a' - 'a')])));
    quick_widget->rootContext()->setContextProperty("_color_b", QVariant::fromValue(convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * ('b' - 'a')])));
    quick_widget->rootContext()->setContextProperty("_color_c", QVariant::fromValue(convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * ('c' - 'a')])));
    quick_widget->rootContext()->setContextProperty("_color_d", QVariant::fromValue(convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * ('d' - 'a')])));
    quick_widget->rootContext()->setContextProperty("_index", selected_symbol - 'a');

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchHighlightButtons.qml"));


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(deleteButtonClicked()),
        this,
        SLOT(handleDelete()));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(changeColorClicked(int)),
        this,
        SLOT(handleChangeColor(int)));


}
void TouchHighlightButtons::setHighlightType(char type) {
    quick_widget->rootContext()->setContextProperty("_index", type - 'a');
}

void TouchHighlightButtons::handleDelete() {
    emit deletePressed();
}

void TouchHighlightButtons::handleChangeColor(int index) {
    emit changeColorPressed(index);
}

void TouchHighlightButtons::resizeEvent(QResizeEvent* resize_event){
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}
