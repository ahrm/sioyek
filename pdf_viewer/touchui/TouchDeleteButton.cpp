#include "touchui/TouchDeleteButton.h"


TouchDeleteButton::TouchDeleteButton(QWidget* parent) : QWidget(parent) {

    //    quick_widget = new QQuickWidget(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"), this);
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    //quick_widget->rootContext()->setContextProperty("_initialValue", initial_value);
    //quick_widget->rootContext()->setContextProperty("_name", name);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchDeleteButton.qml"));


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(deleteButtonClicked()),
        this,
        SLOT(handleDelete()));


}

void TouchDeleteButton::handleDelete() {
    emit deletePressed();
}

void TouchDeleteButton::resizeEvent(QResizeEvent* resize_event) {
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}
