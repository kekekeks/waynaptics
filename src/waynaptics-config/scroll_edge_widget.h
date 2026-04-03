#pragma once

#include <QWidget>
#include <QVector>

struct TouchContact;

class ScrollEdgeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ScrollEdgeWidget(QWidget *parent = nullptr);

    void setEdges(int left, int right, int top, int bottom);
    void setDimensions(int minX, int maxX, int minY, int maxY);
    void setTouchContacts(const QVector<TouchContact> &contacts);

    // Control which edge zones are visible
    void setVerticalEdgesVisible(bool visible);
    void setHorizontalEdgesVisible(bool visible);

    QSize sizeHint() const override;
    int heightForWidth(int w) const override;
    bool hasHeightForWidth() const override { return true; }

signals:
    void edgesChanged(int left, int right, int top, int bottom);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    enum DragMode { None, DragLeft, DragRight, DragTop, DragBottom };

    QRectF padRect() const;
    DragMode hitTest(const QPointF &pos) const;
    int pctFromX(qreal x) const;
    int pctFromY(qreal y) const;
    double aspectRatio() const;

    int m_left = 15, m_right = 85, m_top = 15, m_bottom = 85;

    DragMode m_dragging = None;

    QVector<TouchContact> m_touchContacts;
    int m_dimMinX = 0, m_dimMaxX = 1, m_dimMinY = 0, m_dimMaxY = 1;

    bool m_showVertEdges = true;
    bool m_showHorizEdges = true;

    static constexpr int kEdgeHitRadius = 8;
    static constexpr int kPadMargin = 10;
    static constexpr int kMinHeight = 300;
};
