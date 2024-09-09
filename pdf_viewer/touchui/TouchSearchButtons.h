#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchSearchButtons : public QWidget {
    Q_OBJECT
public:
    TouchSearchButtons(QWidget* parent = nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handlePrevious();
    void handleNext();
    void handleInitial();

signals:
    void previousPressed();
    void nextPressed();
    void initialPressed();

private:
    QQuickWidget* quick_widget = nullptr;

};
