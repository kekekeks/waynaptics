#include "pointer_tab.h"
#include "../config_model.h"
#include "../double_slider.h"
#include "../int_slider.h"

#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>

PointerTab::PointerTab(ConfigModel *model, QWidget *parent)
    : QWidget(parent)
    , m_model(model)
{
    auto *layout = new QVBoxLayout(this);

    // Speed group
    auto *speedGroup = new QGroupBox(tr("Pointer Speed"), this);
    auto *speedLayout = new QFormLayout(speedGroup);

    m_minSpeed = new DoubleSlider(0.0, 255.0, 2, 0.1, true, speedGroup);
    m_minSpeed->setToolTip(tr("Speed factor when moving finger very slowly"));
    speedLayout->addRow(tr("Minimum speed:"), m_minSpeed);

    m_maxSpeed = new DoubleSlider(0.0, 255.0, 2, 0.1, true, speedGroup);
    m_maxSpeed->setToolTip(tr("Speed factor when moving finger very fast"));
    speedLayout->addRow(tr("Maximum speed:"), m_maxSpeed);

    m_accelFactor = new DoubleSlider(0.0, 1.0, 4, 0.001, true, speedGroup);
    m_accelFactor->setToolTip(tr("Acceleration factor for normal pointer movements"));
    speedLayout->addRow(tr("Acceleration factor:"), m_accelFactor);

    layout->addWidget(speedGroup);

    // Pressure motion group
    auto *pressureGroup = new QGroupBox(tr("Pressure Motion"), this);
    auto *pressureLayout = new QFormLayout(pressureGroup);

    bool hasPressure = model->hasCap("PressureDetection");
    if (!hasPressure) {
        pressureGroup->setEnabled(false);
        pressureGroup->setToolTip(tr("Hardware does not support pressure detection"));
    }

    m_pressureMotionMinZ = new IntSlider(1, 255, pressureGroup);
    m_pressureMotionMinZ->setToolTip(tr("Pressure at which minimum motion factor applies"));
    pressureLayout->addRow(tr("Min pressure threshold:"), m_pressureMotionMinZ);

    m_pressureMotionMaxZ = new IntSlider(1, 255, pressureGroup);
    m_pressureMotionMaxZ->setToolTip(tr("Pressure at which maximum motion factor applies"));
    pressureLayout->addRow(tr("Max pressure threshold:"), m_pressureMotionMaxZ);

    m_pressureMotionMinFactor = new DoubleSlider(0.0, 10.0, 2, 0.1, pressureGroup);
    m_pressureMotionMinFactor->setToolTip(tr("Speed multiplier at minimum pressure"));
    pressureLayout->addRow(tr("Min pressure factor:"), m_pressureMotionMinFactor);

    m_pressureMotionMaxFactor = new DoubleSlider(0.0, 10.0, 2, 0.1, pressureGroup);
    m_pressureMotionMaxFactor->setToolTip(tr("Speed multiplier at maximum pressure"));
    pressureLayout->addRow(tr("Max pressure factor:"), m_pressureMotionMaxFactor);

    layout->addWidget(pressureGroup);
    layout->addStretch();

    populate();

    // Connect signals with min/max pairing
    connect(m_minSpeed, &DoubleSlider::valueChanged, [this](double v) {
        m_model->setDoubleValue("MinSpeed", v);
        if (v > m_maxSpeed->value())
            m_maxSpeed->setValue(v);
    });
    connect(m_maxSpeed, &DoubleSlider::valueChanged, [this](double v) {
        m_model->setDoubleValue("MaxSpeed", v);
        if (v < m_minSpeed->value())
            m_minSpeed->setValue(v);
    });
    connect(m_accelFactor, &DoubleSlider::valueChanged, [this](double v) { m_model->setDoubleValue("AccelFactor", v); });
    connect(m_pressureMotionMinZ, &IntSlider::valueChanged, [this](int v) {
        m_model->setIntValue("PressureMotionMinZ", v);
        if (v > m_pressureMotionMaxZ->value())
            m_pressureMotionMaxZ->setValue(v);
    });
    connect(m_pressureMotionMaxZ, &IntSlider::valueChanged, [this](int v) {
        m_model->setIntValue("PressureMotionMaxZ", v);
        if (v < m_pressureMotionMinZ->value())
            m_pressureMotionMinZ->setValue(v);
    });
    connect(m_pressureMotionMinFactor, &DoubleSlider::valueChanged, [this](double v) { m_model->setDoubleValue("PressureMotionMinFactor", v); });
    connect(m_pressureMotionMaxFactor, &DoubleSlider::valueChanged, [this](double v) { m_model->setDoubleValue("PressureMotionMaxFactor", v); });
}

void PointerTab::populate()
{
    m_minSpeed->setValue(m_model->doubleValue("MinSpeed"));
    m_maxSpeed->setValue(m_model->doubleValue("MaxSpeed"));
    m_accelFactor->setValue(m_model->doubleValue("AccelFactor"));
    m_pressureMotionMinZ->setValue(m_model->intValue("PressureMotionMinZ"));
    m_pressureMotionMaxZ->setValue(m_model->intValue("PressureMotionMaxZ"));
    m_pressureMotionMinFactor->setValue(m_model->doubleValue("PressureMotionMinFactor"));
    m_pressureMotionMaxFactor->setValue(m_model->doubleValue("PressureMotionMaxFactor"));
}
