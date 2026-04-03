#pragma once

#include <QWidget>

class ConfigModel;
class QComboBox;
class QSpinBox;
class QCheckBox;
class QGroupBox;
class TouchpadZoneWidget;
class TouchSubscriber;

class ClickZonesTab : public QWidget
{
    Q_OBJECT

public:
    explicit ClickZonesTab(ConfigModel *model, TouchSubscriber *touchSub,
                           QWidget *parent = nullptr);

private:
    void populate();
    void onRightZoneChanged(int left, int right, int top, int bottom);
    void onMiddleZoneChanged(int left, int right, int top, int bottom);
    void syncZoneWidgetFromSpinboxes();
    void onRightButtonEnabled(bool enabled);
    void onMiddleButtonEnabled(bool enabled);

    ConfigModel *m_model;

    QComboBox *m_clickFinger1;
    QComboBox *m_clickFinger2;
    QComboBox *m_clickFinger3;
    QCheckBox *m_enableRightButton;
    QCheckBox *m_enableMiddleButton;
    QSpinBox *m_rbAreaLeft;
    QSpinBox *m_rbAreaRight;
    QSpinBox *m_rbAreaTop;
    QSpinBox *m_rbAreaBottom;
    QSpinBox *m_mbAreaLeft;
    QSpinBox *m_mbAreaRight;
    QSpinBox *m_mbAreaTop;
    QSpinBox *m_mbAreaBottom;
    TouchpadZoneWidget *m_zoneWidget;
    bool m_updating = false;
};
