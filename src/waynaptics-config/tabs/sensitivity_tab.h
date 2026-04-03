#pragma once

#include <QWidget>

class ConfigModel;
class QCheckBox;
class IntSlider;
class QGroupBox;

class SensitivityTab : public QWidget
{
    Q_OBJECT

public:
    explicit SensitivityTab(ConfigModel *model, QWidget *parent = nullptr);

private:
    void populate();

    ConfigModel *m_model;

    IntSlider *m_fingerLow;
    IntSlider *m_fingerHigh;
    QCheckBox *m_palmDetect;
    IntSlider *m_palmMinZ;
    IntSlider *m_palmMinWidth;
    IntSlider *m_horizHysteresis;
    IntSlider *m_vertHysteresis;
};
