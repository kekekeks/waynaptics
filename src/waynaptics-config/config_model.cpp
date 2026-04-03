#include "config_model.h"
#include "socket_client.h"

ConfigModel::ConfigModel(SocketClient *client, bool ignoreCaps, QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_ignoreCaps(ignoreCaps)
{
}

bool ConfigModel::load()
{
    QMap<QString, QString> config;
    if (!m_client->fetchConfig(config))
        return false;

    if (!m_ignoreCaps) {
        if (!m_client->fetchCapabilities(m_caps))
            return false;
    }

    m_applied = config;
    m_current = config;

    // Fetch touchpad dimensions
    QMap<QString, QString> dims;
    if (m_client->fetchDimensions(dims)) {
        m_minX = dims.value("MinX").toInt();
        m_maxX = dims.value("MaxX").toInt();
        m_minY = dims.value("MinY").toInt();
        m_maxY = dims.value("MaxY").toInt();
        m_hasDimensions = (m_maxX > m_minX && m_maxY > m_minY);
    }

    m_dirty = false;
    emit dirtyChanged(false);
    emit loaded();
    return true;
}

QString ConfigModel::value(const QString &name) const
{
    return m_current.value(name);
}

int ConfigModel::intValue(const QString &name) const
{
    return m_current.value(name).toInt();
}

double ConfigModel::doubleValue(const QString &name) const
{
    return m_current.value(name).toDouble();
}

bool ConfigModel::boolValue(const QString &name) const
{
    return m_current.value(name).toInt() != 0;
}

void ConfigModel::setValue(const QString &name, const QString &value)
{
    if (m_current.value(name) == value)
        return;
    m_current[name] = value;
    markDirty();
}

void ConfigModel::setIntValue(const QString &name, int value)
{
    setValue(name, QString::number(value));
}

void ConfigModel::setDoubleValue(const QString &name, double value)
{
    setValue(name, QString::number(value, 'g', 10));
}

void ConfigModel::setBoolValue(const QString &name, bool value)
{
    setValue(name, value ? QStringLiteral("1") : QStringLiteral("0"));
}

bool ConfigModel::hasCap(const QString &name) const
{
    if (m_ignoreCaps)
        return true;
    return m_caps.value(name).toInt() != 0;
}

QMap<QString, QString> ConfigModel::dirtyValues() const
{
    QMap<QString, QString> result;
    for (auto it = m_current.constBegin(); it != m_current.constEnd(); ++it) {
        if (m_applied.value(it.key()) != it.value())
            result[it.key()] = it.value();
    }
    return result;
}

void ConfigModel::clearDirty()
{
    m_applied = m_current;
    m_dirty = false;
    emit dirtyChanged(false);
}

void ConfigModel::resetToApplied()
{
    m_current = m_applied;
    m_dirty = false;
    emit dirtyChanged(false);
    emit loaded();
}

void ConfigModel::markDirty()
{
    bool wasDirty = m_dirty;
    m_dirty = (m_current != m_applied);
    if (wasDirty != m_dirty)
        emit dirtyChanged(m_dirty);
}
