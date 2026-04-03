#include "scrolling_tab.h"
#include "../config_model.h"
#include "../scroll_edge_widget.h"
#include "../touch_subscriber.h"

#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QEvent>
#include <QScrollBar>
#include <QCoreApplication>

// Blocks mouse wheel on spinboxes/comboboxes, forwards to scroll bar
class WheelBlocker : public QObject
{
public:
    explicit WheelBlocker(QAbstractScrollArea *scrollArea)
        : QObject(scrollArea), m_scrollArea(scrollArea) {}
    bool eventFilter(QObject *, QEvent *e) override {
        if (e->type() == QEvent::Wheel) {
            if (auto *bar = m_scrollArea->verticalScrollBar())
                QCoreApplication::sendEvent(bar, e);
            return true;
        }
        return false;
    }
private:
    QAbstractScrollArea *m_scrollArea;
};

// Convert device coordinate to visual percentage (0-100)
static int deviceToVisualEdge(int deviceVal, int minDev, int maxDev)
{
    int range = maxDev - minDev;
    if (range <= 0)
        return deviceVal;
    return qBound(0, (int)qRound(100.0 * (deviceVal - minDev) / range), 100);
}

// Convert visual percentage (0-100) to device coordinate
static int visualToDeviceEdge(int visualPct, int minDev, int maxDev)
{
    int range = maxDev - minDev;
    if (range <= 0)
        return visualPct;
    return minDev + (int)qRound((double)visualPct * range / 100.0);
}

ScrollingTab::ScrollingTab(ConfigModel *model, TouchSubscriber *touchSub,
                           QWidget *parent)
    : QWidget(parent)
    , m_model(model)
{
    auto *layout = new QVBoxLayout(this);

    // Wrap scrollable content
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    auto *scrollContent = new QWidget(scrollArea);
    auto *contentLayout = new QVBoxLayout(scrollContent);

    // --- Two-finger scrolling group (first) ---
    auto *twoFingerGroup = new QGroupBox(tr("Two-Finger Scrolling"), scrollContent);
    auto *twoFingerLayout = new QFormLayout(twoFingerGroup);

    m_vertTwoFingerScroll = new QCheckBox(tr("Vertical two-finger scrolling"), twoFingerGroup);
    m_horizTwoFingerScroll = new QCheckBox(tr("Horizontal two-finger scrolling"), twoFingerGroup);
    twoFingerLayout->addRow(m_vertTwoFingerScroll);
    twoFingerLayout->addRow(m_horizTwoFingerScroll);

    bool hasTwoFinger = model->hasCap("TwoFingerDetection");
    if (!hasTwoFinger) {
        twoFingerGroup->setEnabled(false);
        twoFingerGroup->setToolTip(tr("Hardware does not support two-finger detection"));
    }
    contentLayout->addWidget(twoFingerGroup);

    // --- Edge scrolling group (second) ---
    auto *edgeGroup = new QGroupBox(tr("Edge Scrolling"), scrollContent);
    auto *edgeLayout = new QFormLayout(edgeGroup);

    m_vertEdgeScroll = new QCheckBox(tr("Vertical edge scrolling"), edgeGroup);
    m_horizEdgeScroll = new QCheckBox(tr("Horizontal edge scrolling"), edgeGroup);
    m_cornerCoasting = new QCheckBox(tr("Corner coasting"), edgeGroup);
    edgeLayout->addRow(m_vertEdgeScroll);
    edgeLayout->addRow(m_horizEdgeScroll);
    edgeLayout->addRow(m_cornerCoasting);

    contentLayout->addWidget(edgeGroup);

    // --- Edge boundaries group (third) — visual widget + spinboxes ---
    m_edgeBoundGroup = new QGroupBox(tr("Edge Boundaries"), scrollContent);
    auto *edgeBoundLayout = new QHBoxLayout(m_edgeBoundGroup);

    m_edgeWidget = new ScrollEdgeWidget(m_edgeBoundGroup);
    m_edgeWidget->setDimensions(model->minX(), model->maxX(),
                                model->minY(), model->maxY());
    edgeBoundLayout->addWidget(m_edgeWidget, 1);

    if (touchSub) {
        connect(touchSub, &TouchSubscriber::touchesUpdated,
                m_edgeWidget, &ScrollEdgeWidget::setTouchContacts);
    }

    // Spinboxes on the right
    auto *spinLayout = new QFormLayout();

    m_leftEdge = new QSpinBox(m_edgeBoundGroup);
    m_leftEdge->setRange(0, 100000);
    m_leftEdgeLabel = new QLabel(tr("Left:"));
    spinLayout->addRow(m_leftEdgeLabel, m_leftEdge);

    m_rightEdge = new QSpinBox(m_edgeBoundGroup);
    m_rightEdge->setRange(0, 100000);
    m_rightEdgeLabel = new QLabel(tr("Right:"));
    spinLayout->addRow(m_rightEdgeLabel, m_rightEdge);

    m_topEdge = new QSpinBox(m_edgeBoundGroup);
    m_topEdge->setRange(0, 100000);
    m_topEdgeLabel = new QLabel(tr("Top:"));
    spinLayout->addRow(m_topEdgeLabel, m_topEdge);

    m_bottomEdge = new QSpinBox(m_edgeBoundGroup);
    m_bottomEdge->setRange(0, 100000);
    m_bottomEdgeLabel = new QLabel(tr("Bottom:"));
    spinLayout->addRow(m_bottomEdgeLabel, m_bottomEdge);

    edgeBoundLayout->addLayout(spinLayout);

    contentLayout->addWidget(m_edgeBoundGroup);

    // --- Scroll sensitivity group ---
    auto *sensGroup = new QGroupBox(tr("Scroll Sensitivity"), scrollContent);
    auto *sensLayout = new QFormLayout(sensGroup);

    m_reverseVertScroll = new QCheckBox(tr("Reverse vertical scrolling"), sensGroup);
    sensLayout->addRow(m_reverseVertScroll);

    m_vertScrollDelta = new QSpinBox(sensGroup);
    m_vertScrollDelta->setRange(1, 1000);
    m_vertScrollDelta->setToolTip(tr("Finger distance per scroll event"));
    sensLayout->addRow(tr("Vertical scroll delta:"), m_vertScrollDelta);

    m_reverseHorizScroll = new QCheckBox(tr("Reverse horizontal scrolling"), sensGroup);
    sensLayout->addRow(m_reverseHorizScroll);

    m_horizScrollDelta = new QSpinBox(sensGroup);
    m_horizScrollDelta->setRange(1, 1000);
    m_horizScrollDelta->setToolTip(tr("Finger distance per scroll event"));
    sensLayout->addRow(tr("Horizontal scroll delta:"), m_horizScrollDelta);

    contentLayout->addWidget(sensGroup);

    // --- Coasting / inertial scrolling group ---
    auto *coastGroup = new QGroupBox(tr("Coasting"), scrollContent);
    auto *coastLayout = new QFormLayout(coastGroup);

    m_inertialScroll = new QCheckBox(tr("Inertial scrolling"), coastGroup);
    m_inertialScroll->setToolTip(tr("Continue scrolling with momentum after finger is lifted"));
    coastLayout->addRow(m_inertialScroll);

    m_coastingSpeed = new QDoubleSpinBox(coastGroup);
    m_coastingSpeed->setRange(0.1, 255.0);
    m_coastingSpeed->setDecimals(1);
    m_coastingSpeed->setToolTip(tr("Scrolls/sec threshold to start coasting"));
    coastLayout->addRow(tr("Coasting speed:"), m_coastingSpeed);

    m_coastingFriction = new QDoubleSpinBox(coastGroup);
    m_coastingFriction->setRange(0.0, 255.0);
    m_coastingFriction->setDecimals(1);
    m_coastingFriction->setToolTip(tr("Scrolls/sec² deceleration"));
    coastLayout->addRow(tr("Coasting friction:"), m_coastingFriction);

    contentLayout->addWidget(coastGroup);

    // --- Circular scrolling group ---
    auto *circGroup = new QGroupBox(tr("Circular Scrolling"), scrollContent);
    auto *circLayout = new QFormLayout(circGroup);

    m_circularScrolling = new QCheckBox(tr("Enable circular scrolling"), circGroup);
    circLayout->addRow(m_circularScrolling);

    m_circScrollDelta = new QDoubleSpinBox(circGroup);
    m_circScrollDelta->setRange(0.01, 3.15);
    m_circScrollDelta->setDecimals(2);
    m_circScrollDelta->setSingleStep(0.1);
    m_circScrollDelta->setToolTip(tr("Angle (radians) of finger movement per scroll event"));
    circLayout->addRow(tr("Scroll angle (rad):"), m_circScrollDelta);

    m_circScrollTrigger = new QComboBox(circGroup);
    m_circScrollTrigger->addItem(tr("All Edges"), 0);
    m_circScrollTrigger->addItem(tr("Top Edge"), 1);
    m_circScrollTrigger->addItem(tr("Top Right Corner"), 2);
    m_circScrollTrigger->addItem(tr("Right Edge"), 3);
    m_circScrollTrigger->addItem(tr("Bottom Right Corner"), 4);
    m_circScrollTrigger->addItem(tr("Bottom Edge"), 5);
    m_circScrollTrigger->addItem(tr("Bottom Left Corner"), 6);
    m_circScrollTrigger->addItem(tr("Left Edge"), 7);
    m_circScrollTrigger->addItem(tr("Top Left Corner"), 8);
    circLayout->addRow(tr("Trigger region:"), m_circScrollTrigger);

    contentLayout->addWidget(circGroup);

    contentLayout->addStretch();
    scrollArea->setWidget(scrollContent);
    layout->addWidget(scrollArea);

    // Block mouse wheel on spinboxes/comboboxes
    auto *wb = new WheelBlocker(scrollArea);
    for (auto *sb : std::initializer_list<QAbstractSpinBox*>{
             m_vertScrollDelta, m_horizScrollDelta,
             m_leftEdge, m_rightEdge, m_topEdge, m_bottomEdge,
             m_circScrollDelta, m_coastingSpeed, m_coastingFriction}) {
        sb->setFocusPolicy(Qt::StrongFocus);
        sb->installEventFilter(wb);
    }
    m_circScrollTrigger->setFocusPolicy(Qt::StrongFocus);
    m_circScrollTrigger->installEventFilter(wb);

    populate();

    // Connect signals to model
    connect(m_vertEdgeScroll, &QCheckBox::toggled, [this](bool v) {
        m_model->setBoolValue("VertEdgeScroll", v);
        updateEdgeBoundVisibility();
    });
    connect(m_horizEdgeScroll, &QCheckBox::toggled, [this](bool v) {
        m_model->setBoolValue("HorizEdgeScroll", v);
        updateEdgeBoundVisibility();
    });
    connect(m_cornerCoasting, &QCheckBox::toggled, [this](bool v) { m_model->setBoolValue("CornerCoasting", v); });
    connect(m_vertTwoFingerScroll, &QCheckBox::toggled, [this](bool v) { m_model->setBoolValue("VertTwoFingerScroll", v); });
    connect(m_horizTwoFingerScroll, &QCheckBox::toggled, [this](bool v) { m_model->setBoolValue("HorizTwoFingerScroll", v); });
    connect(m_vertScrollDelta, &QSpinBox::valueChanged, [this](int v) {
        m_model->setIntValue("VertScrollDelta", m_reverseVertScroll->isChecked() ? -v : v);
    });
    connect(m_horizScrollDelta, &QSpinBox::valueChanged, [this](int v) {
        m_model->setIntValue("HorizScrollDelta", m_reverseHorizScroll->isChecked() ? -v : v);
    });
    connect(m_reverseVertScroll, &QCheckBox::toggled, [this](bool rev) {
        int v = m_vertScrollDelta->value();
        m_model->setIntValue("VertScrollDelta", rev ? -v : v);
    });
    connect(m_reverseHorizScroll, &QCheckBox::toggled, [this](bool rev) {
        int v = m_horizScrollDelta->value();
        m_model->setIntValue("HorizScrollDelta", rev ? -v : v);
    });
    connect(m_inertialScroll, &QCheckBox::toggled, [this](bool on) {
        m_coastingSpeed->setEnabled(on);
        m_coastingFriction->setEnabled(on);
        if (!on) {
            m_model->setDoubleValue("CoastingSpeed", 0.0);
        } else if (m_coastingSpeed->value() < 0.1) {
            m_coastingSpeed->setValue(20.0);
            m_model->setDoubleValue("CoastingSpeed", 20.0);
        }
    });
    connect(m_coastingSpeed, &QDoubleSpinBox::valueChanged, [this](double v) { m_model->setDoubleValue("CoastingSpeed", v); });
    connect(m_coastingFriction, &QDoubleSpinBox::valueChanged, [this](double v) { m_model->setDoubleValue("CoastingFriction", v); });
    connect(m_circularScrolling, &QCheckBox::toggled, [this](bool v) {
        m_model->setBoolValue("CircularScrolling", v);
        updateEdgeBoundVisibility();
    });
    connect(m_circScrollDelta, &QDoubleSpinBox::valueChanged, [this](double v) { m_model->setDoubleValue("CircScrollDelta", v); });
    connect(m_circScrollTrigger, &QComboBox::currentIndexChanged, [this](int idx) {
        m_model->setIntValue("CircScrollTrigger", m_circScrollTrigger->itemData(idx).toInt());
    });

    // Edge spinboxes → model + widget sync
    connect(m_leftEdge, &QSpinBox::valueChanged, [this](int v) {
        m_model->setIntValue("LeftEdge", v);
        if (!m_updating) syncEdgeWidgetFromSpinboxes();
    });
    connect(m_rightEdge, &QSpinBox::valueChanged, [this](int v) {
        m_model->setIntValue("RightEdge", v);
        if (!m_updating) syncEdgeWidgetFromSpinboxes();
    });
    connect(m_topEdge, &QSpinBox::valueChanged, [this](int v) {
        m_model->setIntValue("TopEdge", v);
        if (!m_updating) syncEdgeWidgetFromSpinboxes();
    });
    connect(m_bottomEdge, &QSpinBox::valueChanged, [this](int v) {
        m_model->setIntValue("BottomEdge", v);
        if (!m_updating) syncEdgeWidgetFromSpinboxes();
    });

    // Widget drag → spinboxes + model
    connect(m_edgeWidget, &ScrollEdgeWidget::edgesChanged,
            this, &ScrollingTab::onEdgesChanged);
}

void ScrollingTab::populate()
{
    m_updating = true;

    m_vertEdgeScroll->setChecked(m_model->boolValue("VertEdgeScroll"));
    m_horizEdgeScroll->setChecked(m_model->boolValue("HorizEdgeScroll"));
    m_cornerCoasting->setChecked(m_model->boolValue("CornerCoasting"));
    m_vertTwoFingerScroll->setChecked(m_model->boolValue("VertTwoFingerScroll"));
    m_horizTwoFingerScroll->setChecked(m_model->boolValue("HorizTwoFingerScroll"));

    int vDelta = m_model->intValue("VertScrollDelta");
    m_reverseVertScroll->setChecked(vDelta < 0);
    m_vertScrollDelta->setValue(qAbs(vDelta));

    int hDelta = m_model->intValue("HorizScrollDelta");
    m_reverseHorizScroll->setChecked(hDelta < 0);
    m_horizScrollDelta->setValue(qAbs(hDelta));

    double coastSpeed = m_model->doubleValue("CoastingSpeed");
    m_inertialScroll->setChecked(coastSpeed > 0.0);
    m_coastingSpeed->setEnabled(coastSpeed > 0.0);
    m_coastingFriction->setEnabled(coastSpeed > 0.0);
    if (coastSpeed > 0.0)
        m_coastingSpeed->setValue(coastSpeed);
    else
        m_coastingSpeed->setValue(20.0);
    m_coastingFriction->setValue(m_model->doubleValue("CoastingFriction"));

    m_circularScrolling->setChecked(m_model->boolValue("CircularScrolling"));
    m_circScrollDelta->setValue(m_model->doubleValue("CircScrollDelta"));

    int trigger = m_model->intValue("CircScrollTrigger");
    int idx = m_circScrollTrigger->findData(trigger);
    if (idx >= 0)
        m_circScrollTrigger->setCurrentIndex(idx);

    m_leftEdge->setValue(m_model->intValue("LeftEdge"));
    m_rightEdge->setValue(m_model->intValue("RightEdge"));
    m_topEdge->setValue(m_model->intValue("TopEdge"));
    m_bottomEdge->setValue(m_model->intValue("BottomEdge"));

    syncEdgeWidgetFromSpinboxes();
    updateEdgeBoundVisibility();

    m_updating = false;
}

void ScrollingTab::onEdgesChanged(int left, int right, int top, int bottom)
{
    if (m_updating) return;
    m_updating = true;

    int minX = m_model->minX(), maxX = m_model->maxX();
    int minY = m_model->minY(), maxY = m_model->maxY();

    int devLeft = visualToDeviceEdge(left, minX, maxX);
    int devRight = visualToDeviceEdge(right, minX, maxX);
    int devTop = visualToDeviceEdge(top, minY, maxY);
    int devBottom = visualToDeviceEdge(bottom, minY, maxY);

    m_leftEdge->setValue(devLeft);
    m_rightEdge->setValue(devRight);
    m_topEdge->setValue(devTop);
    m_bottomEdge->setValue(devBottom);

    m_model->setIntValue("LeftEdge", devLeft);
    m_model->setIntValue("RightEdge", devRight);
    m_model->setIntValue("TopEdge", devTop);
    m_model->setIntValue("BottomEdge", devBottom);

    m_updating = false;
}

void ScrollingTab::syncEdgeWidgetFromSpinboxes()
{
    int minX = m_model->minX(), maxX = m_model->maxX();
    int minY = m_model->minY(), maxY = m_model->maxY();

    int vLeft = deviceToVisualEdge(m_leftEdge->value(), minX, maxX);
    int vRight = deviceToVisualEdge(m_rightEdge->value(), minX, maxX);
    int vTop = deviceToVisualEdge(m_topEdge->value(), minY, maxY);
    int vBottom = deviceToVisualEdge(m_bottomEdge->value(), minY, maxY);

    m_edgeWidget->setEdges(vLeft, vRight, vTop, vBottom);
}

void ScrollingTab::updateEdgeBoundVisibility()
{
    bool vert = m_vertEdgeScroll->isChecked();
    bool horiz = m_horizEdgeScroll->isChecked();
    bool circ = m_circularScrolling->isChecked();

    // Show entire group if any edge-using feature is on
    m_edgeBoundGroup->setVisible(vert || horiz || circ);

    // Show all edges when circular scrolling is on; otherwise per-direction
    bool showLR = vert || circ;
    bool showTB = horiz || circ;

    m_leftEdge->setVisible(showLR);
    m_leftEdgeLabel->setVisible(showLR);
    m_rightEdge->setVisible(showLR);
    m_rightEdgeLabel->setVisible(showLR);

    m_topEdge->setVisible(showTB);
    m_topEdgeLabel->setVisible(showTB);
    m_bottomEdge->setVisible(showTB);
    m_bottomEdgeLabel->setVisible(showTB);

    m_edgeWidget->setVerticalEdgesVisible(showLR);
    m_edgeWidget->setHorizontalEdgesVisible(showTB);
}
