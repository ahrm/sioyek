#include "touchui/TouchListView.h"
#include <QVariant>
#include "ui.h"

#include "mysortfilterproxymodel.h"

void TouchListView::initialize(int selected_index, bool deletable, bool is_tree) {
    setAttribute(Qt::WA_NoMousePropagation);

    proxy_model->setSourceModel(model);

    //    quick_widget = new QQuickWidget(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"), this);
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    //quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    //quick_widget->setClearColor(Qt::transparent);

    quick_widget->rootContext()->setContextProperty("_selected_index", QVariant::fromValue(selected_index));
    quick_widget->rootContext()->setContextProperty("_focus", QVariant::fromValue(false));
    if (is_tree) {
        quick_widget->rootContext()->setContextProperty("_model", QVariant::fromValue(model));
    }
    else {
        quick_widget->rootContext()->setContextProperty("_model", QVariant::fromValue(proxy_model));
    }

    quick_widget->rootContext()->setContextProperty("_deletable", QVariant::fromValue(deletable));
    //    quick_widget->rootContext()->setContextProperty("_from", from);
    //    quick_widget->rootContext()->setContextProperty("_to", to);

    if (is_tree) {
        quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchTreeView.qml"));
    }
    else {
        quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchListView.qml"));
    }


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(itemSelected(QString, int)), this, SLOT(handleSelect(QString, int)));
    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(itemPressAndHold(QString, int)), this, SLOT(handlePressAndHold(QString, int)));
    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(itemDeleted(QString, int)), this, SLOT(handleDelete(QString, int)));
    quick_widget->setFocus();
}

TouchListView::TouchListView(bool is_fuzzy, QAbstractItemModel* items_, int selected_index, QWidget* parent, bool deletable, bool move, bool is_tree) : QWidget(parent) {

    proxy_model = new MySortFilterProxyModel(is_fuzzy, false);
    model = items_;
    if (move) {
        items_->setParent(this);
    }
    initialize(selected_index, deletable, is_tree);
}

TouchListView::TouchListView(bool is_fuzzy, QStringList items_, int selected_index, QWidget* parent, bool deletable) : QWidget(parent) {

    proxy_model = new MySortFilterProxyModel(is_fuzzy, false);
    model = new QStringListModel(items_, this);
    initialize(selected_index, deletable);
}

void TouchListView::handleSelect(QString val, int index) {
    int source_index = proxy_model->mapToSource(proxy_model->index(index, 0)).row();
    emit itemSelected(val, source_index);
}

void TouchListView::handleDelete(QString val, int index) {
    int source_index = proxy_model->mapToSource(proxy_model->index(index, 0)).row();
    emit itemDeleted(val, source_index);
}

void TouchListView::handlePressAndHold(QString val, int index) {
    int source_index = proxy_model->mapToSource(proxy_model->index(index, 0)).row();
    emit itemPressAndHold(val, source_index);
}

void TouchListView::resizeEvent(QResizeEvent* resize_event) {
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}

void TouchListView::set_keyboard_focus() {
    quick_widget->rootContext()->setContextProperty("_focus", QVariant::fromValue(true));
}

void TouchListView::keyPressEvent(QKeyEvent* kevent) {
    if (kevent->key() == Qt::Key_Return) {
        kevent->accept();
        return;
    }
    QWidget::keyPressEvent(kevent);
    //return true;
}
void TouchListView::update_model() {
    quick_widget->rootContext()->setContextProperty("_model", QVariant::fromValue(proxy_model));
}
