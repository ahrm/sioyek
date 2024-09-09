#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickItem>


class TouchAudioButtons : public QWidget {
    Q_OBJECT
public:
    TouchAudioButtons(QWidget* parent = nullptr);
    void resizeEvent(QResizeEvent* resize_event) override;
    void set_rate(float rate);
    void set_playing();
    void set_paused();

public slots:
    void handlePlay();
    void handlePause();
    void handleStop();
    void handleSetRate(qreal rate);
    //void handleIncreaseSpeed();
    //void handleDecreaseSpeed();
    //void handleInitial();

signals:
    void playPressed();
    void pausePressed();
    void stopPressed();
    void rateChanged(qreal rate);
    //void speedIncreasePressed();
    //void speedDecreasePressed();
    //void initialPressed();

private:
    QQuickWidget* quick_widget = nullptr;

};
