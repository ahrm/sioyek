#include "touchui/TouchRangeSelectUI.h"


TouchRangeSelectUI::TouchRangeSelectUI(float initial_top,
                                               float initial_bottom,
                                               QWidget* parent) : QWidget(parent) {

//    quick_widget = new QQuickWidget(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"), this);
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    quick_widget->rootContext()->setContextProperty("_top", initial_top);
    quick_widget->rootContext()->setContextProperty("_bottom", initial_bottom);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchRangeSelectUI.qml"));

    // force a parent resize so we have the correct size
//    parentWidget()->resize(parentWidget()->width(), parentWidget()->height());
//    resize(parentWidget()->width(), parentWidget()->height());


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
                     SIGNAL(confirmPressed(qreal, qreal)),
                     this,
                     SLOT(handleRangeSelected(qreal, qreal)));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
                     SIGNAL(cancelPressed()),
                     this,
                     SLOT(handleRangeCanceled()));

}

void TouchRangeSelectUI::handleRangeSelected(qreal top, qreal bottom) {
    emit rangeSelected(top, bottom);
}

void TouchRangeSelectUI::handleRangeCanceled() {
    emit rangeCanceled();
}


void TouchRangeSelectUI::resizeEvent(QResizeEvent* resize_event){
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
//    quick_widget->rootObject()->setSize(resize_event->size());
    QWidget::resizeEvent(resize_event);

}
