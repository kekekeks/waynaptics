#pragma once

#include <QWidget>

class ConfigModel;
class QCheckBox;
class QSpinBox;
class QComboBox;
class QGroupBox;

class TappingTab : public QWidget
{
    Q_OBJECT

public:
    explicit TappingTab(ConfigModel *model, QWidget *parent = nullptr);

private:
    void populate();

    ConfigModel *m_model;

    QSpinBox *m_maxTapTime;
    QSpinBox *m_maxDoubleTapTime;
    QSpinBox *m_singleTapTimeout;
    QSpinBox *m_maxTapMove;
    QComboBox *m_tapButton1;
    QComboBox *m_tapButton2;
    QComboBox *m_tapButton3;
    QComboBox *m_rtCornerButton;
    QComboBox *m_rbCornerButton;
    QComboBox *m_ltCornerButton;
    QComboBox *m_lbCornerButton;
    QCheckBox *m_tapAndDrag;
    QCheckBox *m_lockedDrags;
    QSpinBox *m_lockedDragTimeout;
};
