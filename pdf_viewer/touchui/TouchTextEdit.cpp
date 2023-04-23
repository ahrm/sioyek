#include "touchui/TouchTextEdit.h"
#include "utils.h"


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
    quick_widget->setFocus();

}


void TouchTextEdit::handleConfirm(QString text) {
    emit confirmed(text);
}

void TouchTextEdit::handleCancel() {
    emit cancelled();
}


void TouchTextEdit::resizeEvent(QResizeEvent* resize_event){
    QWidget::resizeEvent(resize_event);

    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    float five_cm = 5 * logicalDpiX() / 2.54f;

    int w = static_cast<int>(std::min(parent_width * 0.8f, five_cm));
    int h = static_cast<int>(std::min(parent_height * 0.5f, five_cm));

    resize(w, h);
    quick_widget->resize(w, h);
    move((parent_width - w ) / 2, (parent_height - h) / 2);

}

void TouchTextEdit::keyPressEvent(QKeyEvent* kevent) {
    if (kevent->key() == Qt::Key_Return) {
		kevent->accept();
        return;
    }
    QWidget::keyPressEvent(kevent);
    //return true;
}
