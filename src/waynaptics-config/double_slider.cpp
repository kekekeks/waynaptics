#include "double_slider.h"

#include <cmath>

static constexpr int kSliderSteps = 1000;

DoubleSlider::DoubleSlider(double min, double max, int decimals,
                           double step, bool sqrtScale, QWidget *parent)
    : QWidget(parent)
    , m_min(min)
    , m_max(max)
    , m_steps(kSliderSteps)
    , m_sqrtScale(sqrtScale)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setRange(0, m_steps);
    m_slider->setTickPosition(QSlider::TicksBelow);
    layout->addWidget(m_slider, 1);

    m_spin = new QDoubleSpinBox(this);
    m_spin->setRange(min, max);
    m_spin->setDecimals(decimals);
    if (step > 0.0)
        m_spin->setSingleStep(step);
    else
        m_spin->setSingleStep((max - min) / 100.0);
    layout->addWidget(m_spin, 0);

    connect(m_slider, &QSlider::valueChanged, this, &DoubleSlider::onSliderChanged);
    connect(m_spin, &QDoubleSpinBox::valueChanged, this, &DoubleSlider::onSpinChanged);
}

double DoubleSlider::value() const
{
    return m_spin->value();
}

void DoubleSlider::setValue(double v)
{
    m_updating = true;
    m_spin->setValue(v);
    m_slider->setValue(doubleToSlider(v));
    m_updating = false;
}

void DoubleSlider::setRange(double min, double max)
{
    m_min = min;
    m_max = max;
    m_spin->setRange(min, max);
    m_spin->setSingleStep((max - min) / 100.0);
}

void DoubleSlider::onSliderChanged(int pos)
{
    if (m_updating)
        return;
    m_updating = true;
    double v = sliderToDouble(pos);
    m_spin->setValue(v);
    m_updating = false;
    emit valueChanged(v);
}

void DoubleSlider::onSpinChanged(double val)
{
    if (m_updating)
        return;
    m_updating = true;
    m_slider->setValue(doubleToSlider(val));
    m_updating = false;
    emit valueChanged(val);
}

// Sqrt interpolation: slider position maps to sqrt of value ratio.
// This gives finer control at low values and coarser at high values.
int DoubleSlider::doubleToSlider(double v) const
{
    if (m_max <= m_min)
        return 0;
    double ratio = (v - m_min) / (m_max - m_min);
    if (m_sqrtScale)
        ratio = std::sqrt(ratio);
    return qBound(0, static_cast<int>(ratio * m_steps + 0.5), m_steps);
}

double DoubleSlider::sliderToDouble(int pos) const
{
    double ratio = static_cast<double>(pos) / m_steps;
    if (m_sqrtScale)
        ratio = ratio * ratio;
    return m_min + ratio * (m_max - m_min);
}
