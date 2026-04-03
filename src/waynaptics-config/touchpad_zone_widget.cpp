#include "touchpad_zone_widget.h"
#include "touch_subscriber.h"

#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>
#include <algorithm>

TouchpadZoneWidget::TouchpadZoneWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(200, 150);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
}

void TouchpadZoneWidget::setRightZone(int left, int right, int top, int bottom)
{
    m_rbLeft = left; m_rbRight = right; m_rbTop = top; m_rbBottom = bottom;
    update();
}

void TouchpadZoneWidget::setMiddleZone(int left, int right, int top, int bottom)
{
    m_mbLeft = left; m_mbRight = right; m_mbTop = top; m_mbBottom = bottom;
    update();
}

void TouchpadZoneWidget::setRightEnabled(bool enabled)
{
    m_rbEnabled = enabled;
    update();
}

void TouchpadZoneWidget::setMiddleEnabled(bool enabled)
{
    m_mbEnabled = enabled;
    update();
}

void TouchpadZoneWidget::setDimensions(int minX, int maxX, int minY, int maxY)
{
    m_dimMinX = minX;
    m_dimMaxX = maxX;
    m_dimMinY = minY;
    m_dimMaxY = maxY;
}

void TouchpadZoneWidget::setTouchContacts(const QVector<TouchContact> &contacts)
{
    m_touchContacts = contacts;
    update();
}

QRectF TouchpadZoneWidget::padRect() const
{
    return QRectF(kPadMargin, kPadMargin,
                  width() - 2 * kPadMargin, height() - 2 * kPadMargin);
}

QRectF TouchpadZoneWidget::zoneToRect(int left, int right, int top, int bottom) const
{
    auto pad = padRect();
    qreal x1 = pad.left() + pad.width() * left / 100.0;
    qreal x2 = pad.left() + pad.width() * right / 100.0;
    qreal y1 = pad.top() + pad.height() * top / 100.0;
    qreal y2 = pad.top() + pad.height() * bottom / 100.0;
    return QRectF(QPointF(x1, y1), QPointF(x2, y2)).normalized();
}

int TouchpadZoneWidget::pctFromX(qreal x) const
{
    auto pad = padRect();
    int pct = qRound((x - pad.left()) / pad.width() * 100.0);
    return std::clamp(pct, 0, 100);
}

int TouchpadZoneWidget::pctFromY(qreal y) const
{
    auto pad = padRect();
    int pct = qRound((y - pad.top()) / pad.height() * 100.0);
    return std::clamp(pct, 0, 100);
}

TouchpadZoneWidget::DragMode TouchpadZoneWidget::hitTest(const QPointF &pos) const
{
    auto testZone = [&](const QRectF &rect, DragMode left, DragMode right, DragMode top, DragMode bottom,
                        DragMode tl, DragMode tr, DragMode bl, DragMode br, DragMode move) -> DragMode {
        if (!rect.isValid() || rect.width() < 2 || rect.height() < 2)
            return None;

        bool nearLeft = qAbs(pos.x() - rect.left()) < kCornerHitRadius;
        bool nearRight = qAbs(pos.x() - rect.right()) < kCornerHitRadius;
        bool nearTop = qAbs(pos.y() - rect.top()) < kCornerHitRadius;
        bool nearBottom = qAbs(pos.y() - rect.bottom()) < kCornerHitRadius;
        bool inXRange = pos.x() >= rect.left() - kEdgeHitRadius && pos.x() <= rect.right() + kEdgeHitRadius;
        bool inYRange = pos.y() >= rect.top() - kEdgeHitRadius && pos.y() <= rect.bottom() + kEdgeHitRadius;

        // Corners first (diagonal)
        if (nearLeft && nearTop) return tl;
        if (nearRight && nearTop) return tr;
        if (nearLeft && nearBottom) return bl;
        if (nearRight && nearBottom) return br;

        // Edges
        if (nearLeft && inYRange) return left;
        if (nearRight && inYRange) return right;
        if (nearTop && inXRange) return top;
        if (nearBottom && inXRange) return bottom;

        // Interior — area drag
        if (rect.contains(pos)) return move;

        return None;
    };

    // Test right button zone
    if (m_rbEnabled) {
        auto rbRect = zoneToRect(m_rbLeft, m_rbRight, m_rbTop, m_rbBottom);
        auto rbMode = testZone(rbRect, RBLeft, RBRight, RBTop, RBBottom,
                               RBTopLeft, RBTopRight, RBBottomLeft, RBBottomRight, RBMove);
        if (rbMode != None) return rbMode;
    }

    // Test middle button zone
    if (m_mbEnabled) {
        auto mbRect = zoneToRect(m_mbLeft, m_mbRight, m_mbTop, m_mbBottom);
        auto mbMode = testZone(mbRect, MBLeft, MBRight, MBTop, MBBottom,
                               MBTopLeft, MBTopRight, MBBottomLeft, MBBottomRight, MBMove);
        if (mbMode != None) return mbMode;
    }

    return None;
}

void TouchpadZoneWidget::updateCursor(DragMode mode)
{
    switch (mode) {
    case RBLeft: case RBRight: case MBLeft: case MBRight:
        setCursor(Qt::SizeHorCursor); break;
    case RBTop: case RBBottom: case MBTop: case MBBottom:
        setCursor(Qt::SizeVerCursor); break;
    case RBTopLeft: case RBBottomRight: case MBTopLeft: case MBBottomRight:
        setCursor(Qt::SizeFDiagCursor); break;
    case RBTopRight: case RBBottomLeft: case MBTopRight: case MBBottomLeft:
        setCursor(Qt::SizeBDiagCursor); break;
    case RBMove: case MBMove:
        setCursor(Qt::SizeAllCursor); break;
    default:
        setCursor(Qt::ArrowCursor); break;
    }
}

void TouchpadZoneWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    auto pad = padRect();

    // Draw touchpad background
    QPainterPath padPath;
    padPath.addRoundedRect(pad, 8, 8);
    p.fillPath(padPath, palette().color(QPalette::Base));
    p.setPen(QPen(palette().color(QPalette::Mid), 2));
    p.drawPath(padPath);

    QFont smallFont = font();
    smallFont.setPointSize(smallFont.pointSize() - 1);

    // Draw middle button zone
    if (m_mbEnabled) {
        auto mbRect = zoneToRect(m_mbLeft, m_mbRight, m_mbTop, m_mbBottom);
        if (mbRect.isValid() && mbRect.width() > 1 && mbRect.height() > 1) {
            p.fillRect(mbRect, QColor(100, 149, 237, 80));
            p.setPen(QPen(QColor(100, 149, 237), 2, Qt::DashLine));
            p.drawRect(mbRect);
            p.setPen(palette().color(QPalette::Text));
            p.setFont(smallFont);
            p.drawText(mbRect, Qt::AlignCenter, tr("Middle"));
        }
    }

    // Draw right button zone
    if (m_rbEnabled) {
        auto rbRect = zoneToRect(m_rbLeft, m_rbRight, m_rbTop, m_rbBottom);
        if (rbRect.isValid() && rbRect.width() > 1 && rbRect.height() > 1) {
            p.fillRect(rbRect, QColor(220, 100, 100, 80));
            p.setPen(QPen(QColor(220, 100, 100), 2, Qt::DashLine));
            p.drawRect(rbRect);
            p.setPen(palette().color(QPalette::Text));
            p.setFont(smallFont);
            p.drawText(rbRect, Qt::AlignCenter, tr("Right"));
        }
    }

    // Label for left click area
    int topMost = 100;
    if (m_rbEnabled && m_rbTop > 0 && m_rbTop < topMost) topMost = m_rbTop;
    if (m_mbEnabled && m_mbTop > 0 && m_mbTop < topMost) topMost = m_mbTop;
    QRectF leftArea(pad.left(), pad.top(), pad.width(), pad.height() * topMost / 100.0);
    if (leftArea.height() > 20) {
        p.setPen(palette().color(QPalette::Text));
        p.setFont(smallFont);
        p.drawText(leftArea, Qt::AlignCenter, tr("Left Click"));
    }

    // Draw touch contacts
    if (!m_touchContacts.isEmpty() && m_dimMaxX > m_dimMinX && m_dimMaxY > m_dimMinY) {
        for (const auto &c : m_touchContacts) {
            qreal nx = (qreal)(c.x - m_dimMinX) / (m_dimMaxX - m_dimMinX);
            qreal ny = (qreal)(c.y - m_dimMinY) / (m_dimMaxY - m_dimMinY);
            qreal cx = pad.left() + nx * pad.width();
            qreal cy = pad.top() + ny * pad.height();
            // Radius proportional to pressure, clamped
            qreal radius = std::clamp(c.z / 10.0, 4.0, 20.0);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(50, 200, 50, 180));
            p.drawEllipse(QPointF(cx, cy), radius, radius);
            // Outline
            p.setPen(QPen(QColor(30, 150, 30), 1.5));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(QPointF(cx, cy), radius, radius);
        }
    }

    // Redraw pad border on top
    p.setPen(QPen(palette().color(QPalette::Mid), 2));
    p.drawPath(padPath);
}

void TouchpadZoneWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = hitTest(event->position());
        if (m_dragging != None) {
            m_dragStart = event->position();
            // Save starting values for area drag
            if (m_dragging == RBMove) {
                m_dragStartLeft = m_rbLeft; m_dragStartRight = m_rbRight;
                m_dragStartTop = m_rbTop; m_dragStartBottom = m_rbBottom;
            } else if (m_dragging == MBMove) {
                m_dragStartLeft = m_mbLeft; m_dragStartRight = m_mbRight;
                m_dragStartTop = m_mbTop; m_dragStartBottom = m_mbBottom;
            }
            event->accept();
        }
    }
}

void TouchpadZoneWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging == None) {
        updateCursor(hitTest(event->position()));
        return;
    }

    int px = pctFromX(event->position().x());
    int py = pctFromY(event->position().y());

    if (m_dragging == RBMove || m_dragging == MBMove) {
        // Area drag — compute delta from start
        auto pad = padRect();
        int dx = qRound((event->position().x() - m_dragStart.x()) / pad.width() * 100.0);
        int dy = qRound((event->position().y() - m_dragStart.y()) / pad.height() * 100.0);

        int &left = (m_dragging == RBMove) ? m_rbLeft : m_mbLeft;
        int &right = (m_dragging == RBMove) ? m_rbRight : m_mbRight;
        int &top = (m_dragging == RBMove) ? m_rbTop : m_mbTop;
        int &bottom = (m_dragging == RBMove) ? m_rbBottom : m_mbBottom;

        int origL = m_dragStartLeft, origR = m_dragStartRight;
        int origT = m_dragStartTop, origB = m_dragStartBottom;
        // Resolve 0 = edge for drag computation
        int effR = origR == 0 ? 100 : origR;
        int effB = origB == 0 ? 100 : origB;

        int newL = std::clamp(origL + dx, 0, 100);
        int newR = std::clamp(effR + dx, 0, 100);
        int newT = std::clamp(origT + dy, 0, 100);
        int newB = std::clamp(effB + dy, 0, 100);

        left = newL; right = newR; top = newT; bottom = newB;
    } else {
        switch (m_dragging) {
        case RBLeft:   m_rbLeft = px; break;
        case RBRight:  m_rbRight = px; break;
        case RBTop:    m_rbTop = py; break;
        case RBBottom: m_rbBottom = py; break;
        case MBLeft:   m_mbLeft = px; break;
        case MBRight:  m_mbRight = px; break;
        case MBTop:    m_mbTop = py; break;
        case MBBottom: m_mbBottom = py; break;
        // Diagonal corners
        case RBTopLeft:     m_rbLeft = px; m_rbTop = py; break;
        case RBTopRight:    m_rbRight = px; m_rbTop = py; break;
        case RBBottomLeft:  m_rbLeft = px; m_rbBottom = py; break;
        case RBBottomRight: m_rbRight = px; m_rbBottom = py; break;
        case MBTopLeft:     m_mbLeft = px; m_mbTop = py; break;
        case MBTopRight:    m_mbRight = px; m_mbTop = py; break;
        case MBBottomLeft:  m_mbLeft = px; m_mbBottom = py; break;
        case MBBottomRight: m_mbRight = px; m_mbBottom = py; break;
        default: break;
        }
    }

    update();

    bool isRB = (m_dragging >= RBLeft && m_dragging <= RBBottomRight) || m_dragging == RBMove;
    bool isMB = (m_dragging >= MBLeft && m_dragging <= MBBottomRight) || m_dragging == MBMove;
    if (isRB)
        emit rightZoneChanged(m_rbLeft, m_rbRight, m_rbTop, m_rbBottom);
    else if (isMB)
        emit middleZoneChanged(m_mbLeft, m_mbRight, m_mbTop, m_mbBottom);
}

void TouchpadZoneWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragging = None;
}
