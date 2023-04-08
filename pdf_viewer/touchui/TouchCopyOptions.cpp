#include "touchui/TouchCopyOptions.h"


TouchCopyOptions::TouchCopyOptions(QWidget* parent) : QWidget(parent){

    setAttribute(Qt::WA_NoMousePropagation);
//    quick_widget = new QQuickWidget(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"), this);
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

//    quick_widget->rootContext()->setContextProperty("_initialValue", initial_value);
//    quick_widget->rootContext()->setContextProperty("_from", from);
//    quick_widget->rootContext()->setContextProperty("_to", to);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchCopyOptions.qml"));


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(copyPressed()), this, SLOT(handleCopyClicked()));
    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(scholarPressed()), this, SLOT(handleScholarClicked()));
    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(googlePressed()), this, SLOT(handleGoogleClicked()));
    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(highlightPressed()), this, SLOT(handleHighlightClicked()));

}

void TouchCopyOptions::handleCopyClicked() {
    emit copyClicked();
}

void TouchCopyOptions::handleScholarClicked() {
    emit scholarClicked();
}

void TouchCopyOptions::handleGoogleClicked() {
    emit googleClicked();
}

void TouchCopyOptions::handleHighlightClicked() {
    emit highlightClicked();
}


void TouchCopyOptions::resizeEvent(QResizeEvent* resize_event){
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
//    quick_widget->rootObject()->setSize(resize_event->size());
    QWidget::resizeEvent(resize_event);

}
