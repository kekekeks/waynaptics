#include "clickzones_tab.h"
#include "../config_model.h"
#include "../touchpad_zone_widget.h"
#include "../touch_subscriber.h"

#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

static void setupButtonCombo(QComboBox *combo)
{
    combo->addItem(QObject::tr("Disabled"), 0);
    combo->addItem(QObject::tr("Left Button"), 1);
    combo->addItem(QObject::tr("Middle Button"), 2);
    combo->addItem(QObject::tr("Right Button"), 3);
}

static void setButtonComboValue(QComboBox *combo, int value)
{
    int idx = combo->findData(value);
    if (idx >= 0)
        combo->setCurrentIndex(idx);
}

// Convert device units to visual percentage (0-100).
// For right/bottom edges, device 0 means "extend to edge" = visual 100%.
// For left/top edges, device 0 means "extend to edge" = visual 0%.
static int deviceToVisual(int deviceVal, int minDev, int maxDev, bool isRightOrBottom)
{
    if (deviceVal == 0)
        return isRightOrBottom ? 100 : 0;
    int range = maxDev - minDev;
    if (range <= 0)
        return deviceVal;
    return qBound(0, (int)qRound(100.0 * (deviceVal - minDev) / range), 100);
}

// Convert visual percentage (0-100) to device units.
// Visual 100% for right/bottom = device 0 (extend to edge).
// Visual 0% for left/top = device 0 (extend to edge).
static int visualToDevice(int visualPct, int minDev, int maxDev, bool isRightOrBottom)
{
    if (isRightOrBottom && visualPct == 100)
        return 0;
    if (!isRightOrBottom && visualPct == 0)
        return 0;
    int range = maxDev - minDev;
    if (range <= 0)
        return visualPct;
    return minDev + (int)qRound((double)visualPct * range / 100.0);
}

ClickZonesTab::ClickZonesTab(ConfigModel *model, TouchSubscriber *touchSub,
                             QWidget *parent)
    : QWidget(parent)
    , m_model(model)
{
    auto *layout = new QVBoxLayout(this);

    // Click finger mapping
    auto *clickGroup = new QGroupBox(tr("Click Finger Mapping"), this);
    auto *clickLayout = new QFormLayout(clickGroup);

    m_clickFinger1 = new QComboBox(clickGroup);
    setupButtonCombo(m_clickFinger1);
    clickLayout->addRow(tr("One-finger click:"), m_clickFinger1);

    m_clickFinger2 = new QComboBox(clickGroup);
    setupButtonCombo(m_clickFinger2);
    clickLayout->addRow(tr("Two-finger click:"), m_clickFinger2);
    if (!model->hasCap("TwoFingerDetection")) {
        m_clickFinger2->setEnabled(false);
        m_clickFinger2->setToolTip(tr("Hardware does not support two-finger detection"));
    }

    m_clickFinger3 = new QComboBox(clickGroup);
    setupButtonCombo(m_clickFinger3);
    clickLayout->addRow(tr("Three-finger click:"), m_clickFinger3);
    if (!model->hasCap("ThreeFingerDetection")) {
        m_clickFinger3->setEnabled(false);
        m_clickFinger3->setToolTip(tr("Hardware does not support three-finger detection"));
    }

    layout->addWidget(clickGroup);

    // Soft button areas — visual widget + spinboxes side by side
    auto *areaGroup = new QGroupBox(tr("Soft Button Areas (ClickPad)"), this);
    auto *areaOuterLayout = new QHBoxLayout(areaGroup);

    // Visual zone widget on the left
    m_zoneWidget = new TouchpadZoneWidget(areaGroup);
    m_zoneWidget->setMinimumSize(200, 150);
    m_zoneWidget->setDimensions(model->minX(), model->maxX(),
                                model->minY(), model->maxY());
    areaOuterLayout->addWidget(m_zoneWidget, 2);

    // Connect touch subscriber to draw contacts on the zone widget
    if (touchSub) {
        connect(touchSub, &TouchSubscriber::touchesUpdated,
                m_zoneWidget, &TouchpadZoneWidget::setTouchContacts);
    }

    // Spinboxes on the right for fine-tuning
    auto *spinboxPanel = new QWidget(areaGroup);
    auto *spinLayout = new QVBoxLayout(spinboxPanel);
    spinLayout->setContentsMargins(0, 0, 0, 0);

    auto areaTooltip = tr("Value in device units (0 = extend to edge)");

    // Right button area
    m_enableRightButton = new QCheckBox(tr("Right button area"), spinboxPanel);
    spinLayout->addWidget(m_enableRightButton);
    auto *rbForm = new QFormLayout();

    m_rbAreaLeft = new QSpinBox(spinboxPanel);
    m_rbAreaLeft->setRange(0, 100000);
    m_rbAreaLeft->setToolTip(areaTooltip);
    rbForm->addRow(tr("Left:"), m_rbAreaLeft);

    m_rbAreaRight = new QSpinBox(spinboxPanel);
    m_rbAreaRight->setRange(0, 100000);
    m_rbAreaRight->setToolTip(areaTooltip);
    rbForm->addRow(tr("Right:"), m_rbAreaRight);

    m_rbAreaTop = new QSpinBox(spinboxPanel);
    m_rbAreaTop->setRange(0, 100000);
    m_rbAreaTop->setToolTip(areaTooltip);
    rbForm->addRow(tr("Top:"), m_rbAreaTop);

    m_rbAreaBottom = new QSpinBox(spinboxPanel);
    m_rbAreaBottom->setRange(0, 100000);
    m_rbAreaBottom->setToolTip(areaTooltip);
    rbForm->addRow(tr("Bottom:"), m_rbAreaBottom);

    spinLayout->addLayout(rbForm);

    // Middle button area
    m_enableMiddleButton = new QCheckBox(tr("Middle button area"), spinboxPanel);
    spinLayout->addWidget(m_enableMiddleButton);
    auto *mbForm = new QFormLayout();

    m_mbAreaLeft = new QSpinBox(spinboxPanel);
    m_mbAreaLeft->setRange(0, 100000);
    m_mbAreaLeft->setToolTip(areaTooltip);
    mbForm->addRow(tr("Left:"), m_mbAreaLeft);

    m_mbAreaRight = new QSpinBox(spinboxPanel);
    m_mbAreaRight->setRange(0, 100000);
    m_mbAreaRight->setToolTip(areaTooltip);
    mbForm->addRow(tr("Right:"), m_mbAreaRight);

    m_mbAreaTop = new QSpinBox(spinboxPanel);
    m_mbAreaTop->setRange(0, 100000);
    m_mbAreaTop->setToolTip(areaTooltip);
    mbForm->addRow(tr("Top:"), m_mbAreaTop);

    m_mbAreaBottom = new QSpinBox(spinboxPanel);
    m_mbAreaBottom->setRange(0, 100000);
    m_mbAreaBottom->setToolTip(areaTooltip);
    mbForm->addRow(tr("Bottom:"), m_mbAreaBottom);

    spinLayout->addLayout(mbForm);
    spinLayout->addStretch();

    areaOuterLayout->addWidget(spinboxPanel, 1);

    layout->addWidget(areaGroup, 1);
    layout->addStretch();

    populate();

    // Connect click finger signals
    connect(m_clickFinger1, &QComboBox::currentIndexChanged, [this](int idx) {
        m_model->setIntValue("ClickFinger1", m_clickFinger1->itemData(idx).toInt());
    });
    connect(m_clickFinger2, &QComboBox::currentIndexChanged, [this](int idx) {
        m_model->setIntValue("ClickFinger2", m_clickFinger2->itemData(idx).toInt());
    });
    connect(m_clickFinger3, &QComboBox::currentIndexChanged, [this](int idx) {
        m_model->setIntValue("ClickFinger3", m_clickFinger3->itemData(idx).toInt());
    });

    // Connect enable checkboxes
    connect(m_enableRightButton, &QCheckBox::toggled,
            this, &ClickZonesTab::onRightButtonEnabled);
    connect(m_enableMiddleButton, &QCheckBox::toggled,
            this, &ClickZonesTab::onMiddleButtonEnabled);

    // Connect zone widget signals → spinboxes + model
    connect(m_zoneWidget, &TouchpadZoneWidget::rightZoneChanged,
            this, &ClickZonesTab::onRightZoneChanged);
    connect(m_zoneWidget, &TouchpadZoneWidget::middleZoneChanged,
            this, &ClickZonesTab::onMiddleZoneChanged);

    // Connect spinboxes → zone widget + model
    auto syncFromSpinboxes = [this](int) { syncZoneWidgetFromSpinboxes(); };
    connect(m_rbAreaLeft, &QSpinBox::valueChanged, syncFromSpinboxes);
    connect(m_rbAreaRight, &QSpinBox::valueChanged, syncFromSpinboxes);
    connect(m_rbAreaTop, &QSpinBox::valueChanged, syncFromSpinboxes);
    connect(m_rbAreaBottom, &QSpinBox::valueChanged, syncFromSpinboxes);
    connect(m_mbAreaLeft, &QSpinBox::valueChanged, syncFromSpinboxes);
    connect(m_mbAreaRight, &QSpinBox::valueChanged, syncFromSpinboxes);
    connect(m_mbAreaTop, &QSpinBox::valueChanged, syncFromSpinboxes);
    connect(m_mbAreaBottom, &QSpinBox::valueChanged, syncFromSpinboxes);
}

void ClickZonesTab::onRightButtonEnabled(bool enabled)
{
    m_rbAreaLeft->setEnabled(enabled);
    m_rbAreaRight->setEnabled(enabled);
    m_rbAreaTop->setEnabled(enabled);
    m_rbAreaBottom->setEnabled(enabled);
    m_zoneWidget->setRightEnabled(enabled);

    int minX = m_model->minX(), maxX = m_model->maxX();
    int minY = m_model->minY(), maxY = m_model->maxY();

    m_updating = true;
    if (enabled) {
        // Reasonable defaults: 55% left, 100% right, 80% top, 100% bottom
        int devL = visualToDevice(55, minX, maxX, false);
        int devR = visualToDevice(100, minX, maxX, true);  // 0 = extend to edge
        int devT = visualToDevice(80, minY, maxY, false);
        int devB = visualToDevice(100, minY, maxY, true);  // 0 = extend to edge
        m_rbAreaLeft->setValue(devL);
        m_rbAreaRight->setValue(devR);
        m_rbAreaTop->setValue(devT);
        m_rbAreaBottom->setValue(devB);
        m_zoneWidget->setRightZone(
            deviceToVisual(devL, minX, maxX, false), deviceToVisual(devR, minX, maxX, true),
            deviceToVisual(devT, minY, maxY, false), deviceToVisual(devB, minY, maxY, true));
        m_model->setIntValue("RightButtonAreaLeft", devL);
        m_model->setIntValue("RightButtonAreaRight", devR);
        m_model->setIntValue("RightButtonAreaTop", devT);
        m_model->setIntValue("RightButtonAreaBottom", devB);
    } else {
        m_rbAreaLeft->setValue(0);
        m_rbAreaRight->setValue(0);
        m_rbAreaTop->setValue(0);
        m_rbAreaBottom->setValue(0);
        m_zoneWidget->setRightZone(0, 0, 0, 0);
        m_model->setIntValue("RightButtonAreaLeft", 0);
        m_model->setIntValue("RightButtonAreaRight", 0);
        m_model->setIntValue("RightButtonAreaTop", 0);
        m_model->setIntValue("RightButtonAreaBottom", 0);
    }
    m_updating = false;
}

void ClickZonesTab::onMiddleButtonEnabled(bool enabled)
{
    m_mbAreaLeft->setEnabled(enabled);
    m_mbAreaRight->setEnabled(enabled);
    m_mbAreaTop->setEnabled(enabled);
    m_mbAreaBottom->setEnabled(enabled);
    m_zoneWidget->setMiddleEnabled(enabled);

    int minX = m_model->minX(), maxX = m_model->maxX();
    int minY = m_model->minY(), maxY = m_model->maxY();

    m_updating = true;
    if (enabled) {
        // Reasonable defaults: 45% left, 55% right, 80% top, 100% bottom
        int devL = visualToDevice(45, minX, maxX, false);
        int devR = visualToDevice(55, minX, maxX, true);
        int devT = visualToDevice(80, minY, maxY, false);
        int devB = visualToDevice(100, minY, maxY, true);  // 0 = extend to edge
        m_mbAreaLeft->setValue(devL);
        m_mbAreaRight->setValue(devR);
        m_mbAreaTop->setValue(devT);
        m_mbAreaBottom->setValue(devB);
        m_zoneWidget->setMiddleZone(
            deviceToVisual(devL, minX, maxX, false), deviceToVisual(devR, minX, maxX, true),
            deviceToVisual(devT, minY, maxY, false), deviceToVisual(devB, minY, maxY, true));
        m_model->setIntValue("MiddleButtonAreaLeft", devL);
        m_model->setIntValue("MiddleButtonAreaRight", devR);
        m_model->setIntValue("MiddleButtonAreaTop", devT);
        m_model->setIntValue("MiddleButtonAreaBottom", devB);
    } else {
        m_mbAreaLeft->setValue(0);
        m_mbAreaRight->setValue(0);
        m_mbAreaTop->setValue(0);
        m_mbAreaBottom->setValue(0);
        m_zoneWidget->setMiddleZone(0, 0, 0, 0);
        m_model->setIntValue("MiddleButtonAreaLeft", 0);
        m_model->setIntValue("MiddleButtonAreaRight", 0);
        m_model->setIntValue("MiddleButtonAreaTop", 0);
        m_model->setIntValue("MiddleButtonAreaBottom", 0);
    }
    m_updating = false;
}

void ClickZonesTab::onRightZoneChanged(int left, int right, int top, int bottom)
{
    int minX = m_model->minX(), maxX = m_model->maxX();
    int minY = m_model->minY(), maxY = m_model->maxY();

    // Zone widget gives visual %, convert to device for spinboxes and model
    int devL = visualToDevice(left, minX, maxX, false);
    int devR = visualToDevice(right, minX, maxX, true);
    int devT = visualToDevice(top, minY, maxY, false);
    int devB = visualToDevice(bottom, minY, maxY, true);

    m_updating = true;
    m_rbAreaLeft->setValue(devL);
    m_rbAreaRight->setValue(devR);
    m_rbAreaTop->setValue(devT);
    m_rbAreaBottom->setValue(devB);
    m_model->setIntValue("RightButtonAreaLeft", devL);
    m_model->setIntValue("RightButtonAreaRight", devR);
    m_model->setIntValue("RightButtonAreaTop", devT);
    m_model->setIntValue("RightButtonAreaBottom", devB);
    m_updating = false;
}

void ClickZonesTab::onMiddleZoneChanged(int left, int right, int top, int bottom)
{
    int minX = m_model->minX(), maxX = m_model->maxX();
    int minY = m_model->minY(), maxY = m_model->maxY();

    int devL = visualToDevice(left, minX, maxX, false);
    int devR = visualToDevice(right, minX, maxX, true);
    int devT = visualToDevice(top, minY, maxY, false);
    int devB = visualToDevice(bottom, minY, maxY, true);

    m_updating = true;
    m_mbAreaLeft->setValue(devL);
    m_mbAreaRight->setValue(devR);
    m_mbAreaTop->setValue(devT);
    m_mbAreaBottom->setValue(devB);
    m_model->setIntValue("MiddleButtonAreaLeft", devL);
    m_model->setIntValue("MiddleButtonAreaRight", devR);
    m_model->setIntValue("MiddleButtonAreaTop", devT);
    m_model->setIntValue("MiddleButtonAreaBottom", devB);
    m_updating = false;
}

void ClickZonesTab::syncZoneWidgetFromSpinboxes()
{
    if (m_updating)
        return;

    int minX = m_model->minX(), maxX = m_model->maxX();
    int minY = m_model->minY(), maxY = m_model->maxY();

    // Spinboxes contain device units — convert to visual % for zone widget
    int rbL = m_rbAreaLeft->value(), rbR = m_rbAreaRight->value();
    int rbT = m_rbAreaTop->value(), rbB = m_rbAreaBottom->value();
    int mbL = m_mbAreaLeft->value(), mbR = m_mbAreaRight->value();
    int mbT = m_mbAreaTop->value(), mbB = m_mbAreaBottom->value();

    m_zoneWidget->setRightZone(
        deviceToVisual(rbL, minX, maxX, false), deviceToVisual(rbR, minX, maxX, true),
        deviceToVisual(rbT, minY, maxY, false), deviceToVisual(rbB, minY, maxY, true));
    m_zoneWidget->setMiddleZone(
        deviceToVisual(mbL, minX, maxX, false), deviceToVisual(mbR, minX, maxX, true),
        deviceToVisual(mbT, minY, maxY, false), deviceToVisual(mbB, minY, maxY, true));

    m_model->setIntValue("RightButtonAreaLeft", rbL);
    m_model->setIntValue("RightButtonAreaRight", rbR);
    m_model->setIntValue("RightButtonAreaTop", rbT);
    m_model->setIntValue("RightButtonAreaBottom", rbB);
    m_model->setIntValue("MiddleButtonAreaLeft", mbL);
    m_model->setIntValue("MiddleButtonAreaRight", mbR);
    m_model->setIntValue("MiddleButtonAreaTop", mbT);
    m_model->setIntValue("MiddleButtonAreaBottom", mbB);
}

void ClickZonesTab::populate()
{
    setButtonComboValue(m_clickFinger1, m_model->intValue("ClickFinger1"));
    setButtonComboValue(m_clickFinger2, m_model->intValue("ClickFinger2"));
    setButtonComboValue(m_clickFinger3, m_model->intValue("ClickFinger3"));

    int minX = m_model->minX(), maxX = m_model->maxX();
    int minY = m_model->minY(), maxY = m_model->maxY();

    m_updating = true;

    // Device units from model
    int rbL = m_model->intValue("RightButtonAreaLeft");
    int rbR = m_model->intValue("RightButtonAreaRight");
    int rbT = m_model->intValue("RightButtonAreaTop");
    int rbB = m_model->intValue("RightButtonAreaBottom");
    bool rbEnabled = (rbL != 0 || rbR != 0 || rbT != 0 || rbB != 0);

    m_enableRightButton->setChecked(rbEnabled);
    m_rbAreaLeft->setEnabled(rbEnabled);
    m_rbAreaRight->setEnabled(rbEnabled);
    m_rbAreaTop->setEnabled(rbEnabled);
    m_rbAreaBottom->setEnabled(rbEnabled);
    // Spinboxes show device units directly
    m_rbAreaLeft->setValue(rbL);
    m_rbAreaRight->setValue(rbR);
    m_rbAreaTop->setValue(rbT);
    m_rbAreaBottom->setValue(rbB);
    // Zone widget shows visual %
    m_zoneWidget->setRightEnabled(rbEnabled);
    m_zoneWidget->setRightZone(
        deviceToVisual(rbL, minX, maxX, false), deviceToVisual(rbR, minX, maxX, true),
        deviceToVisual(rbT, minY, maxY, false), deviceToVisual(rbB, minY, maxY, true));

    int mbL = m_model->intValue("MiddleButtonAreaLeft");
    int mbR = m_model->intValue("MiddleButtonAreaRight");
    int mbT = m_model->intValue("MiddleButtonAreaTop");
    int mbB = m_model->intValue("MiddleButtonAreaBottom");
    bool mbEnabled = (mbL != 0 || mbR != 0 || mbT != 0 || mbB != 0);

    m_enableMiddleButton->setChecked(mbEnabled);
    m_mbAreaLeft->setEnabled(mbEnabled);
    m_mbAreaRight->setEnabled(mbEnabled);
    m_mbAreaTop->setEnabled(mbEnabled);
    m_mbAreaBottom->setEnabled(mbEnabled);
    m_mbAreaLeft->setValue(mbL);
    m_mbAreaRight->setValue(mbR);
    m_mbAreaTop->setValue(mbT);
    m_mbAreaBottom->setValue(mbB);
    m_zoneWidget->setMiddleEnabled(mbEnabled);
    m_zoneWidget->setMiddleZone(
        deviceToVisual(mbL, minX, maxX, false), deviceToVisual(mbR, minX, maxX, true),
        deviceToVisual(mbT, minY, maxY, false), deviceToVisual(mbB, minY, maxY, true));

    m_updating = false;
}
