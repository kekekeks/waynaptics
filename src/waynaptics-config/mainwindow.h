#pragma once

#include <QMainWindow>

class QTabWidget;
class QPushButton;
class QStatusBar;
class QSplitter;
class SocketClient;
class ConfigModel;
class TestArea;
class TouchSubscriber;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QString &socketPath, bool ignoreCaps,
                        QWidget *parent = nullptr);

    bool isConnected() const { return m_connected; }

private slots:
    void onApply();
    void onReset();
    void onResetToConfig();
    void onSaveToConfig();
    void onDirtyChanged(bool dirty);
    void onLoaded();

private:
    void setupUi();
    void createTabs();
    void connectButtons();
    bool initialLoad();
    void showError(const QString &msg);
    void showStatus(const QString &msg);

    SocketClient *m_client;
    ConfigModel *m_model;
    TouchSubscriber *m_touchSubscriber;

    QTabWidget *m_tabs;
    QPushButton *m_applyBtn;
    QPushButton *m_resetBtn;
    QPushButton *m_resetToConfigBtn;
    QPushButton *m_saveBtn;
    bool m_connected = false;
};
