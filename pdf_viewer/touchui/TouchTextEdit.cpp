#include "touchui/TouchTextEdit.h"


TouchTextEdit::TouchTextEdit(QString name, QString initial_value, QWidget* parent) : QWidget(parent){

    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    quick_widget->rootContext()->setContextProperty("_initialValue", initial_value);
    quick_widget->rootContext()->setContextProperty("_name", name);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchTextEdit.qml"));

    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(confirmed(QString)),
                this,
                SLOT(handleConfirm(QString)));
    QObject::connect(
                dynamic_cast<QObject*>(quick_widget->rootObject()),
                SIGNAL(cancelled()),
                this,
                SLOT(handleCancel()));

}


void TouchTextEdit::handleConfirm(QString text) {
    emit confirmed(text);
}

void TouchTextEdit::handleCancel() {
    emit cancelled();
}

void TouchTextEdit::resizeEvent(QResizeEvent* resize_event){
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}
