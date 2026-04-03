#include "scroll_edge_widget.h"
#include "touch_subscriber.h"

#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>
#include <algorithm>

ScrollEdgeWidget::ScrollEdgeWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(200, kMinHeight);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
}

void ScrollEdgeWidget::setEdges(int left, int right, int top, int bottom)
{
    m_left = left;
    m_right = right;
    m_top = top;
    m_bottom = bottom;
    update();
}

void ScrollEdgeWidget::setDimensions(int minX, int maxX, int minY, int maxY)
{
    m_dimMinX = minX;
    m_dimMaxX = maxX;
    m_dimMinY = minY;
    m_dimMaxY = maxY;
    updateGeometry();
    update();
}

void ScrollEdgeWidget::setTouchContacts(const QVector<TouchContact> &contacts)
{
    m_touchContacts = contacts;
    update();
}

void ScrollEdgeWidget::setVerticalEdgesVisible(bool visible)
{
    m_showVertEdges = visible;
    update();
}

void ScrollEdgeWidget::setHorizontalEdgesVisible(bool visible)
{
    m_showHorizEdges = visible;
    update();
}

double ScrollEdgeWidget::aspectRatio() const
{
    int w = m_dimMaxX - m_dimMinX;
    int h = m_dimMaxY - m_dimMinY;
    if (w > 0 && h > 0)
        return (double)h / w;
    return 0.7; // fallback
}

QSize ScrollEdgeWidget::sizeHint() const
{
    int w = 300;
    int h = qMax(kMinHeight, (int)(w * aspectRatio()) + 2 * kPadMargin);
    return QSize(w, h);
}

int ScrollEdgeWidget::heightForWidth(int w) const
{
    return qMax(kMinHeight, (int)(w * aspectRatio()) + 2 * kPadMargin);
}

void ScrollEdgeWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

QRectF ScrollEdgeWidget::padRect() const
{
    double ar = aspectRatio();
    qreal availW = width() - 2 * kPadMargin;
    qreal availH = height() - 2 * kPadMargin;

    qreal padW, padH;
    if (availW * ar <= availH) {
        padW = availW;
        padH = availW * ar;
    } else {
        padH = availH;
        padW = availH / ar;
    }

    qreal x = kPadMargin + (availW - padW) / 2;
    qreal y = kPadMargin + (availH - padH) / 2;
    return QRectF(x, y, padW, padH);
}

int ScrollEdgeWidget::pctFromX(qreal x) const
{
    auto pad = padRect();
    int pct = qRound((x - pad.left()) / pad.width() * 100.0);
    return std::clamp(pct, 0, 100);
}

int ScrollEdgeWidget::pctFromY(qreal y) const
{
    auto pad = padRect();
    int pct = qRound((y - pad.top()) / pad.height() * 100.0);
    return std::clamp(pct, 0, 100);
}

ScrollEdgeWidget::DragMode ScrollEdgeWidget::hitTest(const QPointF &pos) const
{
    auto pad = padRect();

    // Left edge inner boundary
    if (m_showVertEdges) {
        qreal lx = pad.left() + pad.width() * m_left / 100.0;
        if (qAbs(pos.x() - lx) < kEdgeHitRadius && pos.y() >= pad.top() && pos.y() <= pad.bottom())
            return DragLeft;

        qreal rx = pad.left() + pad.width() * m_right / 100.0;
        if (qAbs(pos.x() - rx) < kEdgeHitRadius && pos.y() >= pad.top() && pos.y() <= pad.bottom())
            return DragRight;
    }

    if (m_showHorizEdges) {
        qreal ty = pad.top() + pad.height() * m_top / 100.0;
        if (qAbs(pos.y() - ty) < kEdgeHitRadius && pos.x() >= pad.left() && pos.x() <= pad.right())
            return DragTop;

        qreal by = pad.top() + pad.height() * m_bottom / 100.0;
        if (qAbs(pos.y() - by) < kEdgeHitRadius && pos.x() >= pad.left() && pos.x() <= pad.right())
            return DragBottom;
    }

    return None;
}

void ScrollEdgeWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    auto pad = padRect();

    // Draw touchpad background
    QPainterPath padPath;
    padPath.addRoundedRect(pad, 8, 8);
    p.fillPath(padPath, palette().color(QPalette::Base));

    // Compute edge positions in pixels
    qreal lx = pad.left() + pad.width() * m_left / 100.0;
    qreal rx = pad.left() + pad.width() * m_right / 100.0;
    qreal ty = pad.top() + pad.height() * m_top / 100.0;
    qreal by = pad.top() + pad.height() * m_bottom / 100.0;

    QColor edgeColor(100, 180, 100, 60);

    // Clip edge regions to pad bounds using intersection
    p.save();
    p.setClipPath(padPath);

    // Left/Right edge zones (vertical edge scrolling)
    if (m_showVertEdges) {
        if (m_left > 0) {
            QRectF leftZone(pad.left(), pad.top(), lx - pad.left(), pad.height());
            p.fillRect(leftZone, edgeColor);
        }
        if (m_right < 100) {
            QRectF rightZone(rx, pad.top(), pad.right() - rx, pad.height());
            p.fillRect(rightZone, edgeColor);
        }
    }
    // Top/Bottom edge zones (horizontal edge scrolling)
    if (m_showHorizEdges) {
        if (m_top > 0) {
            QRectF topZone(pad.left(), pad.top(), pad.width(), ty - pad.top());
            p.fillRect(topZone, edgeColor);
        }
        if (m_bottom < 100) {
            QRectF bottomZone(pad.left(), by, pad.width(), pad.bottom() - by);
            p.fillRect(bottomZone, edgeColor);
        }
    }

    p.restore();

    // Draw edge boundary lines as dashed lines
    QPen dashPen(QColor(60, 140, 60), 2, Qt::DashLine);
    p.setPen(dashPen);

    if (m_showVertEdges) {
        if (m_left > 0 && m_left < 100)
            p.drawLine(QPointF(lx, pad.top()), QPointF(lx, pad.bottom()));
        if (m_right > 0 && m_right < 100)
            p.drawLine(QPointF(rx, pad.top()), QPointF(rx, pad.bottom()));
    }
    if (m_showHorizEdges) {
        if (m_top > 0 && m_top < 100)
            p.drawLine(QPointF(pad.left(), ty), QPointF(pad.right(), ty));
        if (m_bottom > 0 && m_bottom < 100)
            p.drawLine(QPointF(pad.left(), by), QPointF(pad.right(), by));
    }

    // Labels
    QFont smallFont = font();
    smallFont.setPointSize(smallFont.pointSize() - 1);
    p.setFont(smallFont);
    p.setPen(palette().color(QPalette::Text));

    // Edge labels
    if (m_showVertEdges) {
        if (m_left > 5) {
            QRectF leftLabel(pad.left(), ty, lx - pad.left(), by - ty);
            if (leftLabel.width() > 10)
                p.drawText(leftLabel, Qt::AlignCenter, tr("L"));
        }
        if (m_right < 95) {
            QRectF rightLabel(rx, ty, pad.right() - rx, by - ty);
            if (rightLabel.width() > 10)
                p.drawText(rightLabel, Qt::AlignCenter, tr("R"));
        }
    }
    if (m_showHorizEdges) {
        if (m_top > 5) {
            QRectF topLabel(lx, pad.top(), rx - lx, ty - pad.top());
            if (topLabel.height() > 10)
                p.drawText(topLabel, Qt::AlignCenter, tr("T"));
        }
        if (m_bottom < 95) {
            QRectF bottomLabel(lx, by, rx - lx, pad.bottom() - by);
            if (bottomLabel.height() > 10)
                p.drawText(bottomLabel, Qt::AlignCenter, tr("B"));
        }
    }

    // Draw touch contacts
    if (!m_touchContacts.isEmpty() && m_dimMaxX > m_dimMinX && m_dimMaxY > m_dimMinY) {
        for (const auto &c : m_touchContacts) {
            qreal nx = (qreal)(c.x - m_dimMinX) / (m_dimMaxX - m_dimMinX);
            qreal ny = (qreal)(c.y - m_dimMinY) / (m_dimMaxY - m_dimMinY);
            qreal cx = pad.left() + nx * pad.width();
            qreal cy = pad.top() + ny * pad.height();
            qreal radius = std::clamp(c.z / 10.0, 4.0, 20.0);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(50, 200, 50, 180));
            p.drawEllipse(QPointF(cx, cy), radius, radius);
            p.setPen(QPen(QColor(30, 150, 30), 1.5));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(QPointF(cx, cy), radius, radius);
        }
    }

    // Redraw pad border on top
    p.setPen(QPen(palette().color(QPalette::Mid), 2));
    p.drawPath(padPath);
}

void ScrollEdgeWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = hitTest(event->position());
        if (m_dragging != None)
            event->accept();
    }
}

void ScrollEdgeWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging == None) {
        auto mode = hitTest(event->position());
        if (mode == DragLeft || mode == DragRight)
            setCursor(Qt::SizeHorCursor);
        else if (mode == DragTop || mode == DragBottom)
            setCursor(Qt::SizeVerCursor);
        else
            setCursor(Qt::ArrowCursor);
        return;
    }

    int px = pctFromX(event->position().x());
    int py = pctFromY(event->position().y());

    switch (m_dragging) {
    case DragLeft:   m_left = std::clamp(px, 0, m_right - 1); break;
    case DragRight:  m_right = std::clamp(px, m_left + 1, 100); break;
    case DragTop:    m_top = std::clamp(py, 0, m_bottom - 1); break;
    case DragBottom: m_bottom = std::clamp(py, m_top + 1, 100); break;
    default: break;
    }

    update();
    emit edgesChanged(m_left, m_right, m_top, m_bottom);
}

void ScrollEdgeWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragging = None;
}
