#pragma once

#include <QWidget>
#include <QPointF>
#include <deque>
#include <vector>

class MainWidget;

class ColorWheelWidget : public QWidget {
    Q_OBJECT
public:
    ColorWheelWidget(QWidget* parent);

    void show_at(QPoint center, int highlight_index,
                 const std::deque<char>& recently_used);
    void cancel();
    char hovered_type() const;

signals:
    void type_selected(int highlight_index, char new_type);
    void delete_selected(int highlight_index);
    void wheel_dismissed();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    struct Bubble {
        char type;        // 'a'-'z'
        QPointF center;   // widget-relative
        float radius;
        float color[3];
    };

    int target_highlight_index = -1;
    std::vector<Bubble> bubbles;  // 0-8 inner ring, 9-25 outer ring
    int hovered_index = -1;       // -1 = none, -2 = delete button
    QPointF wheel_center;

    void compute_layout(const std::deque<char>& recently_used);
    int bubble_at(QPoint pos) const;  // returns -2 for delete button
    bool delete_button_visible() const;
};
