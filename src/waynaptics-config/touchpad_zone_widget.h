#pragma once

#include <QWidget>
#include <QRectF>
#include <QVector>

struct TouchContact;

class TouchpadZoneWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TouchpadZoneWidget(QWidget *parent = nullptr);

    // Values are percentages (0-100). 0 means "to the edge" for that direction.
    void setRightZone(int left, int right, int top, int bottom);
    void setMiddleZone(int left, int right, int top, int bottom);

    void setRightEnabled(bool enabled);
    void setMiddleEnabled(bool enabled);

    // Set touchpad dimensions for mapping device coordinates to visual
    void setDimensions(int minX, int maxX, int minY, int maxY);

    // Update touch contact positions (in device coordinates)
    void setTouchContacts(const QVector<TouchContact> &contacts);

signals:
    void rightZoneChanged(int left, int right, int top, int bottom);
    void middleZoneChanged(int left, int right, int top, int bottom);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    enum DragMode {
        None,
        // Single edges
        RBLeft, RBRight, RBTop, RBBottom,
        MBLeft, MBRight, MBTop, MBBottom,
        // Corners (diagonal)
        RBTopLeft, RBTopRight, RBBottomLeft, RBBottomRight,
        MBTopLeft, MBTopRight, MBBottomLeft, MBBottomRight,
        // Area drag
        RBMove, MBMove
    };

    QRectF padRect() const;
    QRectF zoneToRect(int left, int right, int top, int bottom) const;
    DragMode hitTest(const QPointF &pos) const;
    int pctFromX(qreal x) const;
    int pctFromY(qreal y) const;
    void updateCursor(DragMode mode);

    // Right button area percentages
    int m_rbLeft = 50, m_rbRight = 0, m_rbTop = 82, m_rbBottom = 0;
    bool m_rbEnabled = false;
    // Middle button area percentages
    int m_mbLeft = 30, m_mbRight = 50, m_mbTop = 82, m_mbBottom = 0;
    bool m_mbEnabled = false;

    DragMode m_dragging = None;
    QPointF m_dragStart;
    int m_dragStartLeft, m_dragStartRight, m_dragStartTop, m_dragStartBottom;

    // Touch contacts
    QVector<TouchContact> m_touchContacts;
    int m_dimMinX = 0, m_dimMaxX = 1, m_dimMinY = 0, m_dimMaxY = 1;

    static constexpr int kCornerHitRadius = 8;
    static constexpr int kEdgeHitRadius = 6;
    static constexpr int kPadMargin = 10;
};
