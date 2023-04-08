#include "touchui/TouchPageSelector.h"


TouchPageSelector::TouchPageSelector(int from, int to, int initial_value, QWidget* parent) : QWidget(parent){

    setAttribute(Qt::WA_NoMousePropagation);
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    quick_widget->rootContext()->setContextProperty("_value", initial_value);
    quick_widget->rootContext()->setContextProperty("_from", from);
    quick_widget->rootContext()->setContextProperty("_to", to);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchPageSelector.qml"));


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(pageSelected(int)), this, SLOT(handleSelect(int)));

}

void TouchPageSelector::handleSelect(int item) {
    emit pageSelected(item);
}

void TouchPageSelector::resizeEvent(QResizeEvent* resize_event){
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}
