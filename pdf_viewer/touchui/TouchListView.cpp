#include "touchui/TouchListView.h"
#include <QVariant>
#include "ui.h"


TouchListView::TouchListView(QStringList items_, QWidget* parent) : QWidget(parent), items(items_), model(items){

//    proxy_model = new MySortFilterProxyModel();
    proxy_model.setSourceModel(&model);

//    quick_widget = new QQuickWidget(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"), this);
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
//    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
//    quick_widget->setClearColor(Qt::transparent);

    quick_widget->rootContext()->setContextProperty("_model", QVariant::fromValue(&proxy_model));
//    quick_widget->rootContext()->setContextProperty("_from", from);
//    quick_widget->rootContext()->setContextProperty("_to", to);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchListView.qml"));


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(itemSelected(QString, int)), this, SLOT(handleSelect(QString, int)));

}

void TouchListView::handleSelect(QString val, int index) {
    int source_index = proxy_model.mapToSource( proxy_model.index(index, 0)).row();
    emit itemSelected(val, source_index);
}

void TouchListView::resizeEvent(QResizeEvent* resize_event){
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}

