#include "mainwindow.h"
#include "socket_client.h"
#include "config_model.h"
#include "test_area.h"
#include "touch_subscriber.h"
#include "tabs/scrolling_tab.h"
#include "tabs/tapping_tab.h"
#include "tabs/pointer_tab.h"
#include "tabs/sensitivity_tab.h"
#include "tabs/clickzones_tab.h"

#include <QTabWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QStatusBar>
#include <QMessageBox>
#include <QWidget>

MainWindow::MainWindow(const QString &socketPath, bool ignoreCaps, QWidget *parent)
    : QMainWindow(parent)
    , m_client(new SocketClient(socketPath))
    , m_model(new ConfigModel(m_client, ignoreCaps, this))
    , m_touchSubscriber(new TouchSubscriber(socketPath, this))
{
    setWindowTitle(tr("Waynaptics Configuration"));
    resize(850, 550);

    setupUi();
    connectButtons();

    connect(m_model, &ConfigModel::dirtyChanged, this, &MainWindow::onDirtyChanged);
    connect(m_model, &ConfigModel::loaded, this, &MainWindow::onLoaded);

    initialLoad();
}

void MainWindow::setupUi()
{
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    auto *splitter = new QSplitter(Qt::Horizontal, central);

    m_tabs = new QTabWidget(splitter);
    splitter->addWidget(m_tabs);

    auto *testArea = new TestArea(splitter);
    splitter->addWidget(testArea);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    layout->addWidget(splitter);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_applyBtn = new QPushButton(tr("&Apply"), central);
    m_resetBtn = new QPushButton(tr("&Reset"), central);
    m_resetToConfigBtn = new QPushButton(tr("Reset to &Config"), central);
    m_saveBtn = new QPushButton(tr("&Save to Config"), central);

    m_applyBtn->setEnabled(false);
    m_resetBtn->setEnabled(false);

    buttonLayout->addWidget(m_applyBtn);
    buttonLayout->addWidget(m_resetBtn);
    buttonLayout->addWidget(m_resetToConfigBtn);
    buttonLayout->addWidget(m_saveBtn);

    layout->addLayout(buttonLayout);
    setCentralWidget(central);

    statusBar()->showMessage(tr("Connecting..."));
}

void MainWindow::createTabs()
{
    // Remove old tabs if reloading
    while (m_tabs->count() > 0) {
        QWidget *w = m_tabs->widget(0);
        m_tabs->removeTab(0);
        delete w;
    }

    m_tabs->addTab(new ScrollingTab(m_model, m_touchSubscriber, m_tabs), tr("Scrolling"));
    m_tabs->addTab(new TappingTab(m_model, m_tabs), tr("Tapping"));
    m_tabs->addTab(new PointerTab(m_model, m_tabs), tr("Pointer Motion"));
    m_tabs->addTab(new SensitivityTab(m_model, m_tabs), tr("Sensitivity"));
    m_tabs->addTab(new ClickZonesTab(m_model, m_touchSubscriber, m_tabs), tr("Click Zones"));
}

void MainWindow::connectButtons()
{
    connect(m_applyBtn, &QPushButton::clicked, this, &MainWindow::onApply);
    connect(m_resetBtn, &QPushButton::clicked, this, &MainWindow::onReset);
    connect(m_resetToConfigBtn, &QPushButton::clicked, this, &MainWindow::onResetToConfig);
    connect(m_saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveToConfig);
}

bool MainWindow::initialLoad()
{
    if (!m_model->load()) {
        showError(tr("Failed to connect to waynaptics daemon:\n%1").arg(m_client->lastError()));
        statusBar()->showMessage(tr("Disconnected"));
        m_connected = false;
        return false;
    }
    statusBar()->showMessage(tr("Connected"));
    m_connected = true;
    m_touchSubscriber->start();
    return true;
}

void MainWindow::onLoaded()
{
    createTabs();
}

void MainWindow::onDirtyChanged(bool dirty)
{
    m_applyBtn->setEnabled(dirty);
    m_resetBtn->setEnabled(dirty);
}

void MainWindow::onApply()
{
    const auto dirty = m_model->dirtyValues();
    QStringList errors;
    for (auto it = dirty.constBegin(); it != dirty.constEnd(); ++it) {
        QString err;
        if (!m_client->setOption(it.key(), it.value(), err))
            errors << QStringLiteral("%1: %2").arg(it.key(), err);
    }

    if (!errors.isEmpty()) {
        showError(tr("Some options failed to apply:\n%1").arg(errors.join("\n")));
    }

    m_model->clearDirty();
    showStatus(tr("Applied %1 option(s)").arg(dirty.size()));
}

void MainWindow::onReset()
{
    m_model->resetToApplied();
    createTabs();
    showStatus(tr("Reset to last applied state"));
}

void MainWindow::onResetToConfig()
{
    QString err;
    if (!m_client->sendReload(err)) {
        showError(tr("Failed to reload config:\n%1").arg(err));
        return;
    }
    if (!m_model->load()) {
        showError(tr("Failed to re-fetch config:\n%1").arg(m_client->lastError()));
        return;
    }
    // Explicitly recreate tabs to ensure UI matches reloaded model
    createTabs();
    showStatus(tr("Reloaded from config file"));
}

void MainWindow::onSaveToConfig()
{
    QString err;
    if (!m_client->sendSave(err)) {
        showError(tr("Failed to save config:\n%1").arg(err));
        return;
    }
    showStatus(tr("Saved to config file"));
}

void MainWindow::showError(const QString &msg)
{
    QMessageBox::warning(this, tr("Error"), msg);
}

void MainWindow::showStatus(const QString &msg)
{
    statusBar()->showMessage(msg, 5000);
}
