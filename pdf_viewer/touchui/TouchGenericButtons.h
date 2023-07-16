#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchGenericButtons : public QWidget {
    Q_OBJECT
public:
    std::vector<std::wstring> options;
    bool is_top = false;
    TouchGenericButtons(std::vector<std::wstring> buttons, bool top, QWidget* parent = nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleButton(int index, QString value);

signals:
    void buttonPressed(int index, std::wstring value);

private:
    QQuickWidget* quick_widget = nullptr;

};
