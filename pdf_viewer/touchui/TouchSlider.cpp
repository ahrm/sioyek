#include "touchui/TouchSlider.h"


TouchSlider::TouchSlider(int from, int to, int initial_value, QWidget* parent) : QWidget(parent){

//    quick_widget = new QQuickWidget(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"), this);
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    quick_widget->rootContext()->setContextProperty("_initialValue", initial_value);
    quick_widget->rootContext()->setContextProperty("_from", from);
    quick_widget->rootContext()->setContextProperty("_to", to);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"));


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(valueSelected(int)), this, SLOT(handleSelect(int)));
    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(canceled()), this, SLOT(handleCancel()));

}

void TouchSlider::handleSelect(int item) {
    emit itemSelected(item);
}

void TouchSlider::handleCancel() {
    emit canceled();
}


void TouchSlider::resizeEvent(QResizeEvent* resize_event){
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}
