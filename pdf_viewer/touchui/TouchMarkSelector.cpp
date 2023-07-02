#include "touchui/TouchMarkSelector.h"


TouchMarkSelector::TouchMarkSelector(QWidget* parent) : QWidget(parent) {

    //    quick_widget = new QQuickWidget(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"), this);
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    //quick_widget->rootContext()->setContextProperty("_initialValue", initial_value);
    //quick_widget->rootContext()->setContextProperty("_name", name);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchMarkSelector.qml"));
    quick_widget->setFocus();


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(markSelected(QString)),
        this,
        SLOT(handleMarkSelected(QString)));


}

void TouchMarkSelector::handleMarkSelected(QString text) {
    emit onMarkSelected(text);
}

void TouchMarkSelector::resizeEvent(QResizeEvent* resize_event) {
    quick_widget->resize(0, 0);
    resize(0, 0);
    QWidget::resizeEvent(resize_event);

}
