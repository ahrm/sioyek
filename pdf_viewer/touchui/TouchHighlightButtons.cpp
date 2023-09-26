#include "touchui/TouchHighlightButtons.h"
#include "utils.h"

extern float HIGHLIGHT_COLORS[26 * 3];

TouchHighlightButtons::TouchHighlightButtons(char selected_symbol, QWidget* parent) : QWidget(parent) {

    //    quick_widget = new QQuickWidget(QUrl("qrc:/pdf_viewer/touchui/TouchSlider.qml"), this);
    setAttribute(Qt::WA_NoMousePropagation);
    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    QList<QColor> colors;
    const int N_COLORS = 26;
    for (int i = 0; i < N_COLORS; i++) {
        colors.push_back(convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * i]));
    }

    quick_widget->rootContext()->setContextProperty("_colors", QVariant::fromValue(colors));
    //quick_widget->rootContext()->setContextProperty("_index", selected_symbol - 'a');
    //quick_widget->rootContext()->setContextProperty("_current_color", convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * (selected_symbol - 'a')]));
    setHighlightType(selected_symbol);
    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchHighlightButtons.qml"));


    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(deleteButtonClicked()),
        this,
        SLOT(handleDelete()));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(editButtonClicked()),
        this,
        SLOT(handleEdit()));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(changeColorClicked(int)),
        this,
        SLOT(handleChangeColor(int)));


}
void TouchHighlightButtons::setHighlightType(char type) {
    int index = 0;
    if (type >= 'a' && type <= 'z') {
        index = type - 'a';
    }
    if (type >= 'A' && type <= 'Z') {
        index = type - 'A';
    }

    quick_widget->rootContext()->setContextProperty("_index", type - 'a');
    quick_widget->rootContext()->setContextProperty("_current_color", convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * index]));
    QMetaObject::invokeMethod(quick_widget->rootObject(), "on_restart");

}

void TouchHighlightButtons::handleDelete() {
    emit deletePressed();
}

void TouchHighlightButtons::handleEdit() {
    emit editPressed();
}

void TouchHighlightButtons::handleChangeColor(int index) {
    emit changeColorPressed(index);
}

void TouchHighlightButtons::resizeEvent(QResizeEvent* resize_event) {
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}
