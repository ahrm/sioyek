#include "touchui/TouchListView.h"
#include <QVariant>
#include "ui.h"


TouchListView::TouchListView(QStringList items_, QWidget* parent, bool deletable) : QWidget(parent), items(items_), model(items){

//    proxy_model = new MySortFilterProxyModel();
    setAttribute(Qt::WA_NoMousePropagation);
    proxy_model.setSourceModel(&model);

//    quick_widget = new QQuickWidget(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"), this);
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    //quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    //quick_widget->setClearColor(Qt::transparent);

	quick_widget->rootContext()->setContextProperty("_focus", QVariant::fromValue(false));
    quick_widget->rootContext()->setContextProperty("_model", QVariant::fromValue(&proxy_model));
    if (deletable) {
		quick_widget->rootContext()->setContextProperty("_deletable", QVariant::fromValue(true));
    }
//    quick_widget->rootContext()->setContextProperty("_from", from);
//    quick_widget->rootContext()->setContextProperty("_to", to);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchListView.qml"));


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(itemSelected(QString, int)), this, SLOT(handleSelect(QString, int)));
    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(itemPressAndHold(QString, int)), this, SLOT(handlePressAndHold(QString, int)));
    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(itemDeleted(QString, int)), this, SLOT(handleDelete(QString, int)));
    quick_widget->setFocus();

}

void TouchListView::handleSelect(QString val, int index) {
    int source_index = proxy_model.mapToSource( proxy_model.index(index, 0)).row();
    emit itemSelected(val, source_index);
}

void TouchListView::handleDelete(QString val, int index) {
    int source_index = proxy_model.mapToSource( proxy_model.index(index, 0)).row();
    emit itemDeleted(val, source_index);
}

void TouchListView::handlePressAndHold(QString val, int index) {
    int source_index = proxy_model.mapToSource( proxy_model.index(index, 0)).row();
    emit itemPressAndHold(val, source_index);
}

void TouchListView::resizeEvent(QResizeEvent* resize_event){
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}

void TouchListView::set_keyboard_focus() {
		quick_widget->rootContext()->setContextProperty("_focus", QVariant::fromValue(true));
}
