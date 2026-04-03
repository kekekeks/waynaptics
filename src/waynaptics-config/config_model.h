#pragma once

#include <QMap>
#include <QObject>
#include <QString>

class SocketClient;

class ConfigModel : public QObject
{
    Q_OBJECT

public:
    explicit ConfigModel(SocketClient *client, bool ignoreCaps, QObject *parent = nullptr);

    bool load();

    QString value(const QString &name) const;
    int intValue(const QString &name) const;
    double doubleValue(const QString &name) const;
    bool boolValue(const QString &name) const;

    void setValue(const QString &name, const QString &value);
    void setIntValue(const QString &name, int value);
    void setDoubleValue(const QString &name, double value);
    void setBoolValue(const QString &name, bool value);

    bool hasCap(const QString &name) const;

    // Touchpad dimensions (axis ranges in device units)
    int minX() const { return m_minX; }
    int maxX() const { return m_maxX; }
    int minY() const { return m_minY; }
    int maxY() const { return m_maxY; }
    bool hasDimensions() const { return m_hasDimensions; }

    bool isDirty() const { return m_dirty; }
    QMap<QString, QString> dirtyValues() const;
    void clearDirty();
    void resetToApplied();

signals:
    void loaded();
    void dirtyChanged(bool dirty);

private:
    void markDirty();

    SocketClient *m_client;
    bool m_ignoreCaps;
    bool m_dirty = false;

    QMap<QString, QString> m_applied;    // last applied/fetched values
    QMap<QString, QString> m_current;    // current UI values
    QMap<QString, QString> m_caps;       // capabilities

    int m_minX = 0, m_maxX = 1;
    int m_minY = 0, m_maxY = 1;
    bool m_hasDimensions = false;
};
