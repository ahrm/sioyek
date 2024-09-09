#include "touchui/TouchSearchButtons.h"


TouchSearchButtons::TouchSearchButtons(QWidget* parent) : QWidget(parent) {

    //    quick_widget = new QQuickWidget(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"), this);
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    //quick_widget->rootContext()->setContextProperty("_initialValue", initial_value);
    //quick_widget->rootContext()->setContextProperty("_name", name);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchSearchButtons.qml"));


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(prevButtonClicked()),
        this,
        SLOT(handlePrevious()));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(nextButtonClicked()),
        this,
        SLOT(handleNext()));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(initialButtonClicked()),
        this,
        SLOT(handleInitial()));


}

void TouchSearchButtons::handlePrevious() {
    emit previousPressed();
}
void TouchSearchButtons::handleNext() {
    emit nextPressed();
}
void TouchSearchButtons::handleInitial() {
    emit initialPressed();
}

void TouchSearchButtons::resizeEvent(QResizeEvent* resize_event) {
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}
