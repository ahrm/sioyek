#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchCopyOptions : public QWidget{
    Q_OBJECT
public:
    TouchCopyOptions( QWidget* parent=nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;

public slots:
    void handleCopyClicked();
    void handleScholarClicked();
    void handleGoogleClicked();
    void handleHighlightClicked();

signals:
    void copyClicked();
    void scholarClicked();
    void googleClicked();
    void highlightClicked();

private:
    QQuickWidget* quick_widget = nullptr;

};
