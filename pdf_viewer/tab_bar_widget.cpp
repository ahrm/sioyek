#include "tab_bar_widget.h"
#include "main_widget.h"
#include "document.h"
#include "utils.h"

#include <QPainter>
#include <QFileInfo>
#include <QToolTip>

extern float STATUS_BAR_COLOR[3];
extern float STATUS_BAR_TEXT_COLOR[3];
extern bool SHOW_TAB_BAR;
extern bool TAB_BAR_AT_TOP;

static QColor color_from_float3(float* c, int alpha = 255) {
    return QColor(c[0] * 255, c[1] * 255, c[2] * 255, alpha);
}

static const int TAB_PADDING = 12;
static const int CLOSE_BTN_SIZE = 12;
static const int CLOSE_BTN_MARGIN = 6;
static const int MIN_TAB_WIDTH = 80;
static const int MAX_TAB_WIDTH = 250;
static const int ACCENT_HEIGHT = 2;

TabBarWidget::TabBarWidget(MainWidget* parent)
    : QWidget(parent), main_widget(parent)
{
    setMouseTracking(true);
}

void TabBarWidget::update_tabs() {
    recompute_layout(width(), height());
    update();
}

void TabBarWidget::recompute_layout(int w, int h) {
    tab_infos.clear();

    auto tabs = main_widget->document_manager->get_tabs();
    int current_index = main_widget->get_current_tab_index();

    if (tabs.empty()) return;

    QFont font = this->font();
    QFontMetrics fm(font);

    // Compute ideal widths, then clamp
    std::vector<int> widths;
    int total_width = 0;
    for (size_t i = 0; i < tabs.size(); i++) {
        QString label = QFileInfo(QString::fromStdWString(tabs[i])).fileName();
        if (label.isEmpty()) label = "untitled";
        int text_w = fm.horizontalAdvance(label);
        int tab_w = text_w + TAB_PADDING * 2 + CLOSE_BTN_SIZE + CLOSE_BTN_MARGIN * 2;
        tab_w = std::max(MIN_TAB_WIDTH, std::min(MAX_TAB_WIDTH, tab_w));
        widths.push_back(tab_w);
        total_width += tab_w;
    }

    // If total exceeds widget width, shrink tabs proportionally
    if (total_width > w && tabs.size() > 0) {
        int per_tab = w / (int)tabs.size();
        per_tab = std::max(MIN_TAB_WIDTH, per_tab);
        for (size_t i = 0; i < widths.size(); i++) {
            widths[i] = per_tab;
        }
        total_width = per_tab * (int)tabs.size();
    }

    // Clamp scroll offset
    int max_scroll = std::max(0, total_width - w);
    scroll_offset = std::max(0, std::min(scroll_offset, max_scroll));

    int x = -scroll_offset;
    for (size_t i = 0; i < tabs.size(); i++) {
        TabInfo info;
        info.path = tabs[i];
        info.label = QFileInfo(QString::fromStdWString(tabs[i])).fileName();
        if (info.label.isEmpty()) info.label = "untitled";
        info.is_active = ((int)i == current_index);
        info.rect = QRect(x, 0, widths[i], h);

        // Close button: right side of tab, vertically centered
        int close_x = x + widths[i] - CLOSE_BTN_SIZE - CLOSE_BTN_MARGIN;
        int close_y = (h - CLOSE_BTN_SIZE) / 2;
        info.close_rect = QRect(close_x, close_y, CLOSE_BTN_SIZE, CLOSE_BTN_SIZE);

        tab_infos.push_back(info);
        x += widths[i];
    }
}

void TabBarWidget::paintEvent(QPaintEvent*) {
    if (tab_infos.empty()) {
        recompute_layout(width(), height());
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor bg_color = color_from_float3(STATUS_BAR_COLOR);
    QColor text_color = color_from_float3(STATUS_BAR_TEXT_COLOR);
    QColor active_bg = bg_color.lighter(150);
    QColor hover_bg = bg_color.lighter(125);
    QColor inactive_text = color_from_float3(STATUS_BAR_TEXT_COLOR, 140);
    QColor accent_color = text_color;
    QColor close_color = color_from_float3(STATUS_BAR_TEXT_COLOR, 100);
    QColor close_hover = text_color;

    // Background
    painter.fillRect(rect(), bg_color);

    for (size_t i = 0; i < tab_infos.size(); i++) {
        const TabInfo& tab = tab_infos[i];

        // Skip tabs entirely outside visible area
        if (tab.rect.right() < 0 || tab.rect.left() > width()) continue;

        // Tab background
        if (tab.is_active) {
            painter.fillRect(tab.rect, active_bg);
        } else if ((int)i == hovered_index) {
            painter.fillRect(tab.rect, hover_bg);
        }

        // Active tab accent line
        if (tab.is_active) {
            QRect accent_rect;
            if (TAB_BAR_AT_TOP) {
                accent_rect = QRect(tab.rect.left(), tab.rect.bottom() - ACCENT_HEIGHT + 1, tab.rect.width(), ACCENT_HEIGHT);
            } else {
                accent_rect = QRect(tab.rect.left(), 0, tab.rect.width(), ACCENT_HEIGHT);
            }
            painter.fillRect(accent_rect, accent_color);
        }

        // Separator line between tabs
        if (i > 0 && !tab.is_active && (i == 0 || !tab_infos[i-1].is_active)) {
            painter.setPen(QPen(color_from_float3(STATUS_BAR_TEXT_COLOR, 40), 1));
            painter.drawLine(tab.rect.left(), tab.rect.top() + 4, tab.rect.left(), tab.rect.bottom() - 4);
        }

        // Label text (truncated with ellipsis)
        painter.setPen(tab.is_active ? text_color : inactive_text);
        QRect text_rect = tab.rect.adjusted(TAB_PADDING, 0, -(CLOSE_BTN_SIZE + CLOSE_BTN_MARGIN * 2), 0);
        QString elided = painter.fontMetrics().elidedText(tab.label, Qt::ElideRight, text_rect.width());
        painter.drawText(text_rect, Qt::AlignVCenter | Qt::AlignLeft, elided);

        // Close button (×)
        bool close_hovered = ((int)i == hovered_index && hover_on_close);
        painter.setPen(close_hovered ? close_hover : close_color);
        QFont close_font = painter.font();
        close_font.setPixelSize(CLOSE_BTN_SIZE);
        close_font.setBold(close_hovered);
        painter.setFont(close_font);
        painter.drawText(tab.close_rect, Qt::AlignCenter, QString::fromUtf8("\u00d7"));
        painter.setFont(font());
    }
}

int TabBarWidget::tab_at(QPoint pos) {
    for (size_t i = 0; i < tab_infos.size(); i++) {
        if (tab_infos[i].rect.contains(pos)) return (int)i;
    }
    return -1;
}

void TabBarWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;

    int idx = tab_at(event->pos());
    if (idx < 0 || idx >= (int)tab_infos.size()) return;

    if (tab_infos[idx].close_rect.contains(event->pos())) {
        // Close this tab
        main_widget->handle_close_tab(tab_infos[idx].path);
    } else {
        // Switch to this tab
        main_widget->handle_goto_tab(tab_infos[idx].path);
    }
}

void TabBarWidget::mouseMoveEvent(QMouseEvent* event) {
    int idx = tab_at(event->pos());
    bool on_close = (idx >= 0 && idx < (int)tab_infos.size() && tab_infos[idx].close_rect.contains(event->pos()));

    if (idx != hovered_index || on_close != hover_on_close) {
        hovered_index = idx;
        hover_on_close = on_close;
        setCursor(idx >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);

        // Tooltip: show full path
        if (idx >= 0 && !on_close) {
            setToolTip(QString::fromStdWString(tab_infos[idx].path));
        } else {
            setToolTip(QString());
        }

        update();
    }
}

void TabBarWidget::leaveEvent(QEvent*) {
    if (hovered_index != -1) {
        hovered_index = -1;
        hover_on_close = false;
        setCursor(Qt::ArrowCursor);
        update();
    }
}

void TabBarWidget::wheelEvent(QWheelEvent* event) {
    scroll_offset -= event->angleDelta().y();
    int total_width = 0;
    for (auto& t : tab_infos) total_width += t.rect.width();
    int max_scroll = std::max(0, total_width - width());
    scroll_offset = std::max(0, std::min(scroll_offset, max_scroll));
    recompute_layout(width(), height());
    update();
}
