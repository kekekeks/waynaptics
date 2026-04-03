#include "int_slider.h"

IntSlider::IntSlider(int min, int max, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setRange(min, max);
    m_slider->setTickPosition(QSlider::TicksBelow);
    layout->addWidget(m_slider, 1);

    m_spin = new QSpinBox(this);
    m_spin->setRange(min, max);
    layout->addWidget(m_spin, 0);

    connect(m_slider, &QSlider::valueChanged, this, &IntSlider::onSliderChanged);
    connect(m_spin, &QSpinBox::valueChanged, this, &IntSlider::onSpinChanged);
}

int IntSlider::value() const
{
    return m_spin->value();
}

void IntSlider::setValue(int v)
{
    m_updating = true;
    m_spin->setValue(v);
    m_slider->setValue(v);
    m_updating = false;
}

void IntSlider::setRange(int min, int max)
{
    m_slider->setRange(min, max);
    m_spin->setRange(min, max);
}

void IntSlider::onSliderChanged(int pos)
{
    if (m_updating)
        return;
    m_updating = true;
    m_spin->setValue(pos);
    m_updating = false;
    emit valueChanged(pos);
}

void IntSlider::onSpinChanged(int val)
{
    if (m_updating)
        return;
    m_updating = true;
    m_slider->setValue(val);
    m_updating = false;
    emit valueChanged(val);
}
