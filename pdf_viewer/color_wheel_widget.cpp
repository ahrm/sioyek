#include "color_wheel_widget.h"
#include "utils.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <cmath>

extern float HIGHLIGHT_COLORS[26 * 3];

float* get_highlight_type_color(char type);

static const int WIDGET_SIZE = 280;
static const float INNER_RING_RADIUS = 55.0f;
static const float OUTER_RING_RADIUS = 105.0f;
static const float INNER_BUBBLE_RADIUS = 18.0f;
static const float OUTER_BUBBLE_RADIUS = 15.0f;
static const int INNER_COUNT = 9;
static const int OUTER_COUNT = 17;

static float luminance(float r, float g, float b) {
    return 0.299f * r + 0.587f * g + 0.114f * b;
}

ColorWheelWidget::ColorWheelWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint | Qt::SubWindow);
    hide();
}

void ColorWheelWidget::show_at(QPoint center, int highlight_index,
                                const std::deque<char>& recently_used) {
    target_highlight_index = highlight_index;
    hovered_index = -1;

    compute_layout(recently_used);

    // Position widget centered on cursor, clamped to parent bounds
    int half = WIDGET_SIZE / 2;
    QPoint top_left = center - QPoint(half, half);
    if (parentWidget()) {
        QRect bounds = parentWidget()->rect();
        top_left.setX(std::max(bounds.left(), std::min(top_left.x(), bounds.right() - WIDGET_SIZE)));
        top_left.setY(std::max(bounds.top(), std::min(top_left.y(), bounds.bottom() - WIDGET_SIZE)));
    }
    move(top_left);
    resize(WIDGET_SIZE, WIDGET_SIZE);

    wheel_center = QPointF(center.x() - top_left.x(), center.y() - top_left.y());

    show();
    raise();
    grabMouse();
}

void ColorWheelWidget::compute_layout(const std::deque<char>& recently_used) {
    bubbles.clear();

    // Build ordered type list: first 9 from recently_used for inner ring
    std::vector<char> inner_types;
    std::vector<char> outer_types;

    for (char c : recently_used) {
        if (c >= 'a' && c <= 'z' && inner_types.size() < INNER_COUNT) {
            inner_types.push_back(c);
        }
    }

    // Fill inner ring if not enough recently used
    for (char c = 'a'; c <= 'z' && (int)inner_types.size() < INNER_COUNT; c++) {
        bool found = false;
        for (char t : inner_types) { if (t == c) { found = true; break; } }
        if (!found) inner_types.push_back(c);
    }

    // Remaining types go to outer ring
    for (char c = 'a'; c <= 'z'; c++) {
        bool in_inner = false;
        for (char t : inner_types) { if (t == c) { in_inner = true; break; } }
        if (!in_inner) outer_types.push_back(c);
    }

    float cx = WIDGET_SIZE / 2.0f;
    float cy = WIDGET_SIZE / 2.0f;

    // Inner ring: 9 bubbles
    for (int i = 0; i < (int)inner_types.size(); i++) {
        float angle = -M_PI / 2.0f + i * (2.0f * M_PI / INNER_COUNT);
        Bubble b;
        b.type = inner_types[i];
        b.center = QPointF(cx + INNER_RING_RADIUS * std::cos(angle),
                           cy + INNER_RING_RADIUS * std::sin(angle));
        b.radius = INNER_BUBBLE_RADIUS;
        float* col = get_highlight_type_color(b.type);
        b.color[0] = col[0]; b.color[1] = col[1]; b.color[2] = col[2];
        bubbles.push_back(b);
    }

    // Outer ring: 17 bubbles
    for (int i = 0; i < (int)outer_types.size(); i++) {
        float angle = -M_PI / 2.0f + i * (2.0f * M_PI / OUTER_COUNT);
        Bubble b;
        b.type = outer_types[i];
        b.center = QPointF(cx + OUTER_RING_RADIUS * std::cos(angle),
                           cy + OUTER_RING_RADIUS * std::sin(angle));
        b.radius = OUTER_BUBBLE_RADIUS;
        float* col = get_highlight_type_color(b.type);
        b.color[0] = col[0]; b.color[1] = col[1]; b.color[2] = col[2];
        bubbles.push_back(b);
    }
}

int ColorWheelWidget::bubble_at(QPoint pos) const {
    QPointF p(pos);
    for (int i = 0; i < (int)bubbles.size(); i++) {
        float dx = p.x() - bubbles[i].center.x();
        float dy = p.y() - bubbles[i].center.y();
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist <= bubbles[i].radius) {
            return i;
        }
    }
    return -1;
}

char ColorWheelWidget::hovered_type() const {
    if (hovered_index >= 0 && hovered_index < (int)bubbles.size()) {
        return bubbles[hovered_index].type;
    }
    return '\0';
}

void ColorWheelWidget::cancel() {
    hide();
    releaseMouse();
    hovered_index = -1;
    target_highlight_index = -1;
    emit wheel_dismissed();
}

void ColorWheelWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Circular semi-transparent dark background
    float cx = WIDGET_SIZE / 2.0f;
    float cy = WIDGET_SIZE / 2.0f;
    float bg_radius = OUTER_RING_RADIUS + OUTER_BUBBLE_RADIUS + 10.0f;

    QPainterPath clip_path;
    clip_path.addEllipse(QPointF(cx, cy), bg_radius, bg_radius);
    painter.setClipPath(clip_path);
    painter.fillRect(rect(), QColor(0, 0, 0, 160));

    // Draw bubbles
    for (int i = 0; i < (int)bubbles.size(); i++) {
        const Bubble& b = bubbles[i];
        QColor fill_color(b.color[0] * 255, b.color[1] * 255, b.color[2] * 255);

        // Bubble fill
        painter.setPen(Qt::NoPen);
        painter.setBrush(fill_color);
        painter.drawEllipse(b.center, b.radius, b.radius);

        // Hover ring
        if (i == hovered_index) {
            painter.setPen(QPen(Qt::white, 3.0));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(b.center, b.radius + 1.5, b.radius + 1.5);
        }

        // Letter label
        float lum = luminance(b.color[0], b.color[1], b.color[2]);
        QColor text_color = (lum < 0.5f) ? Qt::white : Qt::black;
        painter.setPen(text_color);

        QFont font = painter.font();
        font.setPixelSize(i < INNER_COUNT ? 13 : 11);
        font.setBold(true);
        painter.setFont(font);

        QRectF text_rect(b.center.x() - b.radius, b.center.y() - b.radius,
                         b.radius * 2, b.radius * 2);
        painter.drawText(text_rect, Qt::AlignCenter, QString(QChar(b.type)));
    }
}

void ColorWheelWidget::mouseMoveEvent(QMouseEvent* event) {
    int idx = bubble_at(event->pos());
    if (idx != hovered_index) {
        hovered_index = idx;
        setCursor(idx >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
}

void ColorWheelWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        if (hovered_index >= 0 && hovered_index < (int)bubbles.size()) {
            emit type_selected(target_highlight_index, bubbles[hovered_index].type);
        }
    }
    cancel();
}
