#include "tapping_tab.h"
#include "../config_model.h"

#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QVBoxLayout>
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

TappingTab::TappingTab(ConfigModel *model, QWidget *parent)
    : QWidget(parent)
    , m_model(model)
{
    auto *layout = new QVBoxLayout(this);

    // Tap timing group
    auto *timingGroup = new QGroupBox(tr("Tap Timing"), this);
    auto *timingLayout = new QFormLayout(timingGroup);

    m_maxTapTime = new QSpinBox(timingGroup);
    m_maxTapTime->setRange(0, 1000);
    m_maxTapTime->setToolTip(tr("Maximum time for detecting a tap"));
    timingLayout->addRow(tr("Max tap time (ms):"), m_maxTapTime);

    m_maxDoubleTapTime = new QSpinBox(timingGroup);
    m_maxDoubleTapTime->setRange(0, 1000);
    m_maxDoubleTapTime->setToolTip(tr("Maximum time for detecting a double tap"));
    timingLayout->addRow(tr("Max double-tap time (ms):"), m_maxDoubleTapTime);

    m_singleTapTimeout = new QSpinBox(timingGroup);
    m_singleTapTimeout->setRange(0, 1000);
    m_singleTapTimeout->setToolTip(tr("Timeout after tap to recognize as single tap"));
    timingLayout->addRow(tr("Single tap timeout (ms):"), m_singleTapTimeout);

    m_maxTapMove = new QSpinBox(timingGroup);
    m_maxTapMove->setRange(0, 1000);
    m_maxTapMove->setToolTip(tr("Maximum finger movement for a tap"));
    timingLayout->addRow(tr("Max tap movement:"), m_maxTapMove);

    layout->addWidget(timingGroup);

    // Tap button mapping group
    auto *mapGroup = new QGroupBox(tr("Tap Button Mapping"), this);
    auto *mapLayout = new QFormLayout(mapGroup);

    m_tapButton1 = new QComboBox(mapGroup);
    setupButtonCombo(m_tapButton1);
    mapLayout->addRow(tr("One-finger tap:"), m_tapButton1);

    m_tapButton2 = new QComboBox(mapGroup);
    setupButtonCombo(m_tapButton2);
    mapLayout->addRow(tr("Two-finger tap:"), m_tapButton2);
    if (!model->hasCap("TwoFingerDetection")) {
        m_tapButton2->setEnabled(false);
        m_tapButton2->setToolTip(tr("Hardware does not support two-finger detection"));
    }

    m_tapButton3 = new QComboBox(mapGroup);
    setupButtonCombo(m_tapButton3);
    mapLayout->addRow(tr("Three-finger tap:"), m_tapButton3);
    if (!model->hasCap("ThreeFingerDetection")) {
        m_tapButton3->setEnabled(false);
        m_tapButton3->setToolTip(tr("Hardware does not support three-finger detection"));
    }

    layout->addWidget(mapGroup);

    // Corner tap mapping group — 2x2 grid
    auto *cornerGroup = new QGroupBox(tr("Corner Tap Mapping"), this);
    auto *cornerLayout = new QGridLayout(cornerGroup);

    m_ltCornerButton = new QComboBox(cornerGroup);
    setupButtonCombo(m_ltCornerButton);
    m_rtCornerButton = new QComboBox(cornerGroup);
    setupButtonCombo(m_rtCornerButton);
    m_lbCornerButton = new QComboBox(cornerGroup);
    setupButtonCombo(m_lbCornerButton);
    m_rbCornerButton = new QComboBox(cornerGroup);
    setupButtonCombo(m_rbCornerButton);

    // [LT]  [RT]
    // [LB]  [RB]
    cornerLayout->addWidget(new QLabel(tr("Top-left:"), cornerGroup), 0, 0);
    cornerLayout->addWidget(m_ltCornerButton, 0, 1);
    cornerLayout->addWidget(new QLabel(tr("Top-right:"), cornerGroup), 0, 2);
    cornerLayout->addWidget(m_rtCornerButton, 0, 3);
    cornerLayout->addWidget(new QLabel(tr("Bottom-left:"), cornerGroup), 1, 0);
    cornerLayout->addWidget(m_lbCornerButton, 1, 1);
    cornerLayout->addWidget(new QLabel(tr("Bottom-right:"), cornerGroup), 1, 2);
    cornerLayout->addWidget(m_rbCornerButton, 1, 3);

    layout->addWidget(cornerGroup);

    // Drag gestures group
    auto *dragGroup = new QGroupBox(tr("Drag Gestures"), this);
    auto *dragLayout = new QFormLayout(dragGroup);

    m_tapAndDrag = new QCheckBox(tr("Enable tap-and-drag gesture"), dragGroup);
    dragLayout->addRow(m_tapAndDrag);

    m_lockedDrags = new QCheckBox(tr("Locked drags"), dragGroup);
    m_lockedDrags->setToolTip(tr("When enabled, drag continues until second tap or timeout"));
    dragLayout->addRow(m_lockedDrags);

    m_lockedDragTimeout = new QSpinBox(dragGroup);
    m_lockedDragTimeout->setRange(0, 30000);
    m_lockedDragTimeout->setToolTip(tr("Auto-release timeout for locked drags"));
    dragLayout->addRow(tr("Locked drag timeout (ms):"), m_lockedDragTimeout);

    layout->addWidget(dragGroup);
    layout->addStretch();

    populate();

    // Connect signals
    connect(m_maxTapTime, &QSpinBox::valueChanged, [this](int v) { m_model->setIntValue("MaxTapTime", v); });
    connect(m_maxDoubleTapTime, &QSpinBox::valueChanged, [this](int v) { m_model->setIntValue("MaxDoubleTapTime", v); });
    connect(m_singleTapTimeout, &QSpinBox::valueChanged, [this](int v) { m_model->setIntValue("SingleTapTimeout", v); });
    connect(m_maxTapMove, &QSpinBox::valueChanged, [this](int v) { m_model->setIntValue("MaxTapMove", v); });
    connect(m_tapButton1, &QComboBox::currentIndexChanged, [this](int idx) {
        m_model->setIntValue("TapButton1", m_tapButton1->itemData(idx).toInt());
    });
    connect(m_tapButton2, &QComboBox::currentIndexChanged, [this](int idx) {
        m_model->setIntValue("TapButton2", m_tapButton2->itemData(idx).toInt());
    });
    connect(m_tapButton3, &QComboBox::currentIndexChanged, [this](int idx) {
        m_model->setIntValue("TapButton3", m_tapButton3->itemData(idx).toInt());
    });
    connect(m_rtCornerButton, &QComboBox::currentIndexChanged, [this](int idx) {
        m_model->setIntValue("RTCornerButton", m_rtCornerButton->itemData(idx).toInt());
    });
    connect(m_rbCornerButton, &QComboBox::currentIndexChanged, [this](int idx) {
        m_model->setIntValue("RBCornerButton", m_rbCornerButton->itemData(idx).toInt());
    });
    connect(m_ltCornerButton, &QComboBox::currentIndexChanged, [this](int idx) {
        m_model->setIntValue("LTCornerButton", m_ltCornerButton->itemData(idx).toInt());
    });
    connect(m_lbCornerButton, &QComboBox::currentIndexChanged, [this](int idx) {
        m_model->setIntValue("LBCornerButton", m_lbCornerButton->itemData(idx).toInt());
    });
    connect(m_tapAndDrag, &QCheckBox::toggled, [this](bool v) { m_model->setBoolValue("TapAndDragGesture", v); });
    connect(m_lockedDrags, &QCheckBox::toggled, [this](bool v) { m_model->setBoolValue("LockedDrags", v); });
    connect(m_lockedDragTimeout, &QSpinBox::valueChanged, [this](int v) { m_model->setIntValue("LockedDragTimeout", v); });
}

void TappingTab::populate()
{
    m_maxTapTime->setValue(m_model->intValue("MaxTapTime"));
    m_maxDoubleTapTime->setValue(m_model->intValue("MaxDoubleTapTime"));
    m_singleTapTimeout->setValue(m_model->intValue("SingleTapTimeout"));
    m_maxTapMove->setValue(m_model->intValue("MaxTapMove"));

    setButtonComboValue(m_tapButton1, m_model->intValue("TapButton1"));
    setButtonComboValue(m_tapButton2, m_model->intValue("TapButton2"));
    setButtonComboValue(m_tapButton3, m_model->intValue("TapButton3"));
    setButtonComboValue(m_rtCornerButton, m_model->intValue("RTCornerButton"));
    setButtonComboValue(m_rbCornerButton, m_model->intValue("RBCornerButton"));
    setButtonComboValue(m_ltCornerButton, m_model->intValue("LTCornerButton"));
    setButtonComboValue(m_lbCornerButton, m_model->intValue("LBCornerButton"));

    m_tapAndDrag->setChecked(m_model->boolValue("TapAndDragGesture"));
    m_lockedDrags->setChecked(m_model->boolValue("LockedDrags"));
    m_lockedDragTimeout->setValue(m_model->intValue("LockedDragTimeout"));
}
