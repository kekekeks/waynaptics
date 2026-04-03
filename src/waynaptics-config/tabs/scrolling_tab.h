#pragma once

#include <QWidget>

class ConfigModel;
class QCheckBox;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QGroupBox;
class QGridLayout;
class QLabel;
class ScrollEdgeWidget;
class TouchSubscriber;

class ScrollingTab : public QWidget
{
    Q_OBJECT

public:
    explicit ScrollingTab(ConfigModel *model, TouchSubscriber *touchSub,
                          QWidget *parent = nullptr);

private:
    void populate();
    void onEdgesChanged(int left, int right, int top, int bottom);
    void syncEdgeWidgetFromSpinboxes();
    void updateEdgeBoundVisibility();

    ConfigModel *m_model;

    QCheckBox *m_vertEdgeScroll;
    QCheckBox *m_horizEdgeScroll;
    QCheckBox *m_cornerCoasting;
    QCheckBox *m_vertTwoFingerScroll;
    QCheckBox *m_horizTwoFingerScroll;
    QCheckBox *m_reverseVertScroll;
    QCheckBox *m_reverseHorizScroll;
    QCheckBox *m_inertialScroll;
    QSpinBox *m_vertScrollDelta;
    QSpinBox *m_horizScrollDelta;
    QCheckBox *m_circularScrolling;
    QDoubleSpinBox *m_circScrollDelta;
    QComboBox *m_circScrollTrigger;
    QDoubleSpinBox *m_coastingSpeed;
    QDoubleSpinBox *m_coastingFriction;
    QSpinBox *m_leftEdge;
    QSpinBox *m_rightEdge;
    QSpinBox *m_topEdge;
    QSpinBox *m_bottomEdge;
    QLabel *m_leftEdgeLabel;
    QLabel *m_rightEdgeLabel;
    QLabel *m_topEdgeLabel;
    QLabel *m_bottomEdgeLabel;
    ScrollEdgeWidget *m_edgeWidget;
    QGroupBox *m_edgeBoundGroup;
    bool m_updating = false;
};
