#include "touchui/TouchAudioButtons.h"


TouchAudioButtons::TouchAudioButtons(QWidget* parent) : QWidget(parent){

//    quick_widget = new QQuickWidget(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"), this);
    setAttribute(Qt::WA_NoMousePropagation);
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    //quick_widget->rootContext()->setContextProperty("_initialValue", initial_value);
    //quick_widget->rootContext()->setContextProperty("_name", name);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchAudioButtons.qml"));


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(playButtonClicked()),
        this,
        SLOT(handlePlay()));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(pauseButtonClicked()),
        this,
        SLOT(handlePause()));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(stopButtonClicked()),
        this,
        SLOT(handleStop()));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(increaseSpeedClicked()),
        this,
        SLOT(handleIncreaseSpeed()));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(decreaseSpeedClicked()),
        this,
        SLOT(handleDecreaseSpeed()));


}

void TouchAudioButtons::handlePlay() {
    emit playPressed();
}

void TouchAudioButtons::handlePause() {
    emit pausePressed();
}

void TouchAudioButtons::handleStop() {
    emit stopPressed();
}

void TouchAudioButtons::resizeEvent(QResizeEvent* resize_event){
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}
void TouchAudioButtons::handleIncreaseSpeed() {

    emit speedIncreasePressed();
}

void TouchAudioButtons::handleDecreaseSpeed() {

    emit speedDecreasePressed();
}
