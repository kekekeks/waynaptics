#pragma once

#include <QWidget>

class ConfigModel;
class DoubleSlider;
class IntSlider;
class QGroupBox;

class PointerTab : public QWidget
{
    Q_OBJECT

public:
    explicit PointerTab(ConfigModel *model, QWidget *parent = nullptr);

private:
    void populate();

    ConfigModel *m_model;

    DoubleSlider *m_minSpeed;
    DoubleSlider *m_maxSpeed;
    DoubleSlider *m_accelFactor;
    IntSlider *m_pressureMotionMinZ;
    IntSlider *m_pressureMotionMaxZ;
    DoubleSlider *m_pressureMotionMinFactor;
    DoubleSlider *m_pressureMotionMaxFactor;
};
