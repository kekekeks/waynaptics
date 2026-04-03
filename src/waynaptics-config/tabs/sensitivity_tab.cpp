#include "sensitivity_tab.h"
#include "../config_model.h"
#include "../int_slider.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>

SensitivityTab::SensitivityTab(ConfigModel *model, QWidget *parent)
    : QWidget(parent)
    , m_model(model)
{
    auto *layout = new QVBoxLayout(this);

    // Finger detection group
    auto *fingerGroup = new QGroupBox(tr("Finger Detection"), this);
    auto *fingerLayout = new QFormLayout(fingerGroup);

    bool hasPressure = model->hasCap("PressureDetection");

    m_fingerLow = new IntSlider(0, 255, fingerGroup);
    m_fingerLow->setToolTip(tr("Pressure below which finger is considered released"));
    fingerLayout->addRow(tr("Finger low (release):"), m_fingerLow);
    if (!hasPressure) {
        m_fingerLow->setEnabled(false);
        m_fingerLow->setToolTip(tr("Hardware does not support pressure detection"));
    }

    m_fingerHigh = new IntSlider(0, 255, fingerGroup);
    m_fingerHigh->setToolTip(tr("Pressure above which finger is considered touching"));
    fingerLayout->addRow(tr("Finger high (touch):"), m_fingerHigh);
    if (!hasPressure) {
        m_fingerHigh->setEnabled(false);
        m_fingerHigh->setToolTip(tr("Hardware does not support pressure detection"));
    }

    layout->addWidget(fingerGroup);

    // Palm detection group
    auto *palmGroup = new QGroupBox(tr("Palm Detection"), this);
    auto *palmLayout = new QFormLayout(palmGroup);

    bool hasPalmWidth = model->hasCap("FingerOrPalmWidthDetection");
    if (!hasPalmWidth) {
        palmGroup->setEnabled(false);
        palmGroup->setToolTip(tr("Hardware does not support finger/palm width detection"));
    }

    m_palmDetect = new QCheckBox(tr("Enable palm detection"), palmGroup);
    m_palmDetect->setToolTip(tr("Requires hardware/firmware support from the touchpad"));
    palmLayout->addRow(m_palmDetect);

    m_palmMinZ = new IntSlider(0, 255, palmGroup);
    m_palmMinZ->setToolTip(tr("Minimum finger pressure to consider as palm"));
    palmLayout->addRow(tr("Palm min pressure:"), m_palmMinZ);

    m_palmMinWidth = new IntSlider(0, 15, palmGroup);
    m_palmMinWidth->setToolTip(tr("Minimum finger width to consider as palm"));
    palmLayout->addRow(tr("Palm min width:"), m_palmMinWidth);

    layout->addWidget(palmGroup);

    // Noise cancellation group
    auto *noiseGroup = new QGroupBox(tr("Noise Cancellation (Hysteresis)"), this);
    auto *noiseLayout = new QFormLayout(noiseGroup);

    m_horizHysteresis = new IntSlider(0, 10000, noiseGroup);
    m_horizHysteresis->setToolTip(tr("Minimum horizontal distance to generate motion (0 = disabled)"));
    noiseLayout->addRow(tr("Horizontal hysteresis:"), m_horizHysteresis);

    m_vertHysteresis = new IntSlider(0, 10000, noiseGroup);
    m_vertHysteresis->setToolTip(tr("Minimum vertical distance to generate motion (0 = disabled)"));
    noiseLayout->addRow(tr("Vertical hysteresis:"), m_vertHysteresis);

    layout->addWidget(noiseGroup);
    layout->addStretch();

    populate();

    // Connect signals
    connect(m_fingerLow, &IntSlider::valueChanged, [this](int v) { m_model->setIntValue("FingerLow", v); });
    connect(m_fingerHigh, &IntSlider::valueChanged, [this](int v) { m_model->setIntValue("FingerHigh", v); });
    connect(m_palmDetect, &QCheckBox::toggled, [this](bool v) { m_model->setBoolValue("PalmDetect", v); });
    connect(m_palmMinZ, &IntSlider::valueChanged, [this](int v) { m_model->setIntValue("PalmMinZ", v); });
    connect(m_palmMinWidth, &IntSlider::valueChanged, [this](int v) { m_model->setIntValue("PalmMinWidth", v); });
    connect(m_horizHysteresis, &IntSlider::valueChanged, [this](int v) { m_model->setIntValue("HorizHysteresis", v); });
    connect(m_vertHysteresis, &IntSlider::valueChanged, [this](int v) { m_model->setIntValue("VertHysteresis", v); });
}

void SensitivityTab::populate()
{
    m_fingerLow->setValue(m_model->intValue("FingerLow"));
    m_fingerHigh->setValue(m_model->intValue("FingerHigh"));
    m_palmDetect->setChecked(m_model->boolValue("PalmDetect"));
    m_palmMinZ->setValue(m_model->intValue("PalmMinZ"));
    m_palmMinWidth->setValue(m_model->intValue("PalmMinWidth"));
    m_horizHysteresis->setValue(m_model->intValue("HorizHysteresis"));
    m_vertHysteresis->setValue(m_model->intValue("VertHysteresis"));
}
