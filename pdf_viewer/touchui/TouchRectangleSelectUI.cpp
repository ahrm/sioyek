#include "touchui/TouchRectangleSelectUI.h"


TouchRectangleSelectUI::TouchRectangleSelectUI(bool initial_enabled,
                                               float initial_x,
                                               float initial_y,
                                               float initial_width,
                                               float initial_height,
                                               QWidget* parent) : QWidget(parent) {

//    quick_widget = new QQuickWidget(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"), this);
    setAttribute(Qt::WA_NoMousePropagation);
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    quick_widget->rootContext()->setContextProperty("_enabled", initial_enabled);
    quick_widget->rootContext()->setContextProperty("_x", initial_x);
    quick_widget->rootContext()->setContextProperty("_y", initial_y);
    quick_widget->rootContext()->setContextProperty("_width", initial_width);
    quick_widget->rootContext()->setContextProperty("_height", initial_height);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchRectangleSelectUI.qml"));


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
                     SIGNAL(rectangleSelected(bool, qreal, qreal, qreal, qreal)),
                     this,
                     SLOT(handleRectangleSelected(bool, qreal, qreal, qreal, qreal)));

}

void TouchRectangleSelectUI::handleRectangleSelected(bool enabled, qreal left, qreal right, qreal top, qreal bottom) {
    emit rectangleSelected(enabled, left, right, top, bottom);
}


void TouchRectangleSelectUI::resizeEvent(QResizeEvent* resize_event){
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
//    quick_widget->rootObject()->setSize(resize_event->size());
    QWidget::resizeEvent(resize_event);

}
