#include "touchui/TouchGenericButtons.h"
#include <qstringlistmodel.h>


TouchGenericButtons::TouchGenericButtons(std::vector<std::wstring> buttons, bool top, QWidget* parent) : QWidget(parent) {

    setAttribute(Qt::WA_NoMousePropagation);

    is_top = top;
    options = buttons;
    QList<QString> q_buttons;

    for (auto button : buttons) {
        q_buttons.append(QString::fromStdWString(button));
    }

    //QStringListModel* buttons_model = new QStringListModel(q_buttons, this);

    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    quick_widget->rootContext()->setContextProperty("_buttons", q_buttons);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchGenericButtons.qml"));

    QObject::connect(dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(genericButtonClicked(int, QString)),
        this,
        SLOT(handleButton(int, QString)));


}

void TouchGenericButtons::handleButton(int index, QString value) {
    emit buttonPressed(index, value.toStdWString());
}

void TouchGenericButtons::resizeEvent(QResizeEvent* resize_event) {

    int parent_width = parentWidget()->size().width();
    int parent_height = parentWidget()->size().height();

    int width = 2 * parent_width / 3;
    int height = parent_height / 12;
    if (width > (150 * options.size())) {
        width = 150 * options.size();
    }

    resize(width, height);
    if (is_top) {
        move(parent_width / 2 - width / 2, height / 2);
    }
    else {
        move(parent_width / 2 - width / 2, parent_height - 3 * height / 2);
    }
    quick_widget->resize(width, height);

    QWidget::resizeEvent(resize_event);

}
