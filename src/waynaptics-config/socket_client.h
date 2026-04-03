#pragma once

#include <QMap>
#include <QString>

class SocketClient
{
public:
    explicit SocketClient(const QString &socketPath);

    bool fetchConfig(QMap<QString, QString> &out);
    bool fetchCapabilities(QMap<QString, QString> &out);
    bool fetchDimensions(QMap<QString, QString> &out);
    bool setOption(const QString &name, const QString &value, QString &error);
    bool sendReload(QString &error);
    bool sendSave(QString &error);

    const QString &lastError() const { return m_lastError; }

private:
    bool sendAndReceiveDump(const QString &command, QMap<QString, QString> &out);
    bool sendAndReceiveStatus(const QString &command, QString &error);

    QString m_socketPath;
    QString m_lastError;
};
