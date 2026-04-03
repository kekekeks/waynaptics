#pragma once

#include <QWidget>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QHBoxLayout>

// A horizontal slider paired with a spinbox for double-precision values.
// Supports optional sqrt interpolation for finer control at low values.
class DoubleSlider : public QWidget
{
    Q_OBJECT

public:
    explicit DoubleSlider(double min, double max, int decimals = 2,
                          double step = 0.0, bool sqrtScale = false,
                          QWidget *parent = nullptr);

    double value() const;
    void setValue(double v);

    void setRange(double min, double max);

signals:
    void valueChanged(double value);

private:
    void onSliderChanged(int pos);
    void onSpinChanged(double val);
    int doubleToSlider(double v) const;
    double sliderToDouble(int pos) const;

    QSlider *m_slider;
    QDoubleSpinBox *m_spin;
    double m_min;
    double m_max;
    int m_steps;
    bool m_sqrtScale;
    bool m_updating = false;
};
