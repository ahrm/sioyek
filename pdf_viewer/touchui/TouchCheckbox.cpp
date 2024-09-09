#include "touchui/TouchCheckbox.h"


TouchCheckbox::TouchCheckbox(QString name, bool initial_value, QWidget* parent) : QWidget(parent) {

    //    quick_widget = new QQuickWidget(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"), this);
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    quick_widget->rootContext()->setContextProperty("_initialValue", initial_value);
    quick_widget->rootContext()->setContextProperty("_name", name);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchCheckbox.qml"));


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(valueSelected(bool)), this, SLOT(handleSelect(bool)));
    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(canceled()), this, SLOT(handleCancel()));

}

void TouchCheckbox::handleSelect(bool item) {
    emit itemSelected(item);
}

void TouchCheckbox::handleCancel() {
    emit canceled();
}

void TouchCheckbox::resizeEvent(QResizeEvent* resize_event) {
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}
