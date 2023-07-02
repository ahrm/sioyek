#include "touchui/TouchConfigMenu.h"
#include <QVariant>
#include "ui.h"
#include "main_widget.h"


TouchConfigMenu::TouchConfigMenu(bool fuzzy, MainWidget* main_widget) :
    QWidget(main_widget),
    config_model(main_widget->config_manager->get_configs_ptr()),
    config_manager(main_widget->config_manager),
    main_widget(main_widget)
{

    setAttribute(Qt::WA_NoMousePropagation);

    proxy_model = new MySortFilterProxyModel(fuzzy);
    proxy_model->setParent(this);

    proxy_model->setSourceModel(&config_model);
    proxy_model->setFilterKeyColumn(1);

    //    quick_widget = new QQuickWidget(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"), this);
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    //quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    //quick_widget->setClearColor(Qt::transparent);

    quick_widget->rootContext()->setContextProperty("_model", QVariant::fromValue(proxy_model));
    quick_widget->rootContext()->setContextProperty("_deletable", QVariant::fromValue(proxy_model));

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchConfigMenu.qml"));


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(color3ConfigChanged(QString, qreal, qreal, qreal)), this, SLOT(handleColor3ConfigChanged(QString, qreal, qreal, qreal)));
    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(color4ConfigChanged(QString, qreal, qreal, qreal, qreal)), this, SLOT(handleColor4ConfigChanged(QString, qreal, qreal, qreal, qreal)));
    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(boolConfigChanged(QString, bool)),
        this,
        SLOT(handleBoolConfigChanged(QString, bool)));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(floatConfigChanged(QString, qreal)),
        this,
        SLOT(handleFloatConfigChanged(QString, qreal)));
    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(textConfigChanged(QString, QString)),
        this,
        SLOT(handleTextConfigChanged(QString, QString)));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(onSetConfigPressed(QString)),
        this,
        SLOT(handleSetConfigPressed(QString)));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(onSaveButtonClicked()),
        this,
        SLOT(handleSaveButtonClicked()));
    //QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(itemPressAndHold(QString, int)), this, SLOT(handlePressAndHold(QString, int)));
    //QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()), SIGNAL(itemDeleted(QString, int)), this, SLOT(handleDelete(QString, int)));

}

void TouchConfigMenu::handleBoolConfigChanged(QString config_name, bool new_value) {
    Config* config = config_manager->get_mut_config_with_name(config_name.toStdWString());
    assert(config->config_type == ConfigType::Bool);
    *((bool*)config->value) = new_value;
}

void TouchConfigMenu::handleFloatConfigChanged(QString config_name, qreal new_value) {
    Config* config = config_manager->get_mut_config_with_name(config_name.toStdWString());
    assert(config->config_type == ConfigType::Float);
    *((float*)config->value) = new_value;
    main_widget->invalidate_render();
}

void TouchConfigMenu::handleTextConfigChanged(QString config_name, QString new_value) {
    Config* config = config_manager->get_mut_config_with_name(config_name.toStdWString());
    assert(config->config_type == ConfigType::String);
    *((std::wstring*)config->value) = new_value.toStdWString();
}

void TouchConfigMenu::handleSetConfigPressed(QString config_name) {
    //command_manager->execute_command(CommandType::SetConfig, config_name.toStdWString());
    auto command = main_widget->command_manager->get_command_with_name(main_widget, (QString("setconfig_") + config_name).toStdString());
    main_widget->handle_command_types(std::move(command), 0);
}

void TouchConfigMenu::handleIntConfigChanged(QString config_name, int new_value) {
    Config* config = config_manager->get_mut_config_with_name(config_name.toStdWString());
    assert(config->config_type == ConfigType::Int);
    *((int*)config->value) = new_value;
    main_widget->invalidate_render();
}

void TouchConfigMenu::handleColor3ConfigChanged(QString config_name, qreal r, qreal g, qreal b) {
    Config* config = config_manager->get_mut_config_with_name(config_name.toStdWString());
    assert(config->config_type == ConfigType::Color3);
    *((float*)config->value + 0) = r;
    *((float*)config->value + 1) = g;
    *((float*)config->value + 2) = b;
    //convert_qcolor_to_float3(QColor(r, g, b), (float*)config->value);
}

void TouchConfigMenu::handleColor4ConfigChanged(QString config_name, qreal r, qreal g, qreal b, qreal a) {
    Config* config = config_manager->get_mut_config_with_name(config_name.toStdWString());
    assert(config->config_type == ConfigType::Color4);
    *((float*)config->value + 0) = r;
    *((float*)config->value + 1) = g;
    *((float*)config->value + 2) = b;
    *((float*)config->value + 3) = a;
}

void TouchConfigMenu::handleSaveButtonClicked() {
    main_widget->persist_config();
    main_widget->pop_current_widget();
}

//
//void TouchListView::handleDelete(QString val, int index) {
//    int source_index = proxy_model.mapToSource( proxy_model.index(index, 0)).row();
//    emit itemDeleted(val, source_index);
//}
//
//void TouchListView::handlePressAndHold(QString val, int index) {
//    int source_index = proxy_model.mapToSource( proxy_model.index(index, 0)).row();
//    emit itemPressAndHold(val, source_index);
//}

void TouchConfigMenu::resizeEvent(QResizeEvent* resize_event) {
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();
    int w = parent_width * 0.8f;
    int h = parent_height * 0.8f;

    quick_widget->resize(w, h);
    move(parent_width * 0.1f, parent_height * 0.1f);
    resize(w, h);
    QWidget::resizeEvent(resize_event);

}

