#include "touchui/TouchMacroEditor.h"
#include "utils.h"
#include <qstandarditemmodel.h>
#include <qsortfilterproxymodel.h>
#include "main_widget.h"

TouchMacroEditor::TouchMacroEditor(std::string macro, QWidget* parent, MainWidget* main_widget) : QWidget(parent){

//    quick_widget = new QQuickWidget(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"), this);
    
    auto commands_model = new QStringListModel(main_widget->command_manager->get_all_command_names(), this);
    QSortFilterProxyModel* proxy_model = new QSortFilterProxyModel(this);
    proxy_model->setSourceModel(commands_model);


    setAttribute(Qt::WA_NoMousePropagation);
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    quick_widget->rootContext()->setContextProperty("_macro", QString::fromStdString(macro));
    quick_widget->rootContext()->setContextProperty("_commands_model", proxy_model);
    quick_widget->rootContext()->setContextProperty("_selected_index", 0);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchMacroEditor.qml"));


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(confirmed(QString)), this, SLOT(handleConfirm(QString)));
    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(canceled()), this, SLOT(handleCancel()));

}

void TouchMacroEditor::handleConfirm(QString macro) {
    emit macroConfirmed(macro.toStdString());
}

void TouchMacroEditor::handleCancel() {
    emit canceled();
}

void TouchMacroEditor::resizeEvent(QResizeEvent* resize_event){
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}
