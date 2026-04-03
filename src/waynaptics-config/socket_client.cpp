#include "socket_client.h"
#include <QLocalSocket>

static constexpr int kTimeoutMs = 3000;

SocketClient::SocketClient(const QString &socketPath)
    : m_socketPath(socketPath)
{
}

bool SocketClient::sendAndReceiveDump(const QString &command, QMap<QString, QString> &out)
{
    QLocalSocket socket;
    socket.connectToServer(m_socketPath);
    if (!socket.waitForConnected(kTimeoutMs)) {
        m_lastError = QStringLiteral("Cannot connect to %1: %2")
                          .arg(m_socketPath, socket.errorString());
        return false;
    }

    QByteArray cmd = (command + "\n").toUtf8();
    socket.write(cmd);
    socket.flush();

    // The daemon closes the connection after dumping, so read until disconnected
    QByteArray response;
    while (socket.waitForReadyRead(kTimeoutMs) || socket.bytesAvailable() > 0) {
        response.append(socket.readAll());
    }
    // Read any remaining data after disconnect
    response.append(socket.readAll());

    out.clear();
    const QList<QByteArray> lines = response.split('\n');
    for (const QByteArray &line : lines) {
        QString l = QString::fromUtf8(line).trimmed();
        if (l.isEmpty())
            continue;
        int eq = l.indexOf('=');
        if (eq < 0)
            continue;
        QString name = l.left(eq).trimmed();
        QString value = l.mid(eq + 1).trimmed();
        out[name] = value;
    }

    return true;
}

bool SocketClient::sendAndReceiveStatus(const QString &command, QString &error)
{
    QLocalSocket socket;
    socket.connectToServer(m_socketPath);
    if (!socket.waitForConnected(kTimeoutMs)) {
        error = QStringLiteral("Cannot connect to %1: %2")
                    .arg(m_socketPath, socket.errorString());
        m_lastError = error;
        return false;
    }

    QByteArray cmd = (command + "\n").toUtf8();
    socket.write(cmd);
    socket.flush();

    if (!socket.waitForReadyRead(kTimeoutMs)) {
        error = QStringLiteral("Timeout waiting for response");
        m_lastError = error;
        return false;
    }

    QString response = QString::fromUtf8(socket.readAll()).trimmed();
    socket.disconnectFromServer();

    if (response.startsWith("OK")) {
        return true;
    }

    if (response.startsWith("FAIL ")) {
        error = response.mid(5);
    } else {
        error = QStringLiteral("Unexpected response: %1").arg(response);
    }
    m_lastError = error;
    return false;
}

bool SocketClient::fetchConfig(QMap<QString, QString> &out)
{
    return sendAndReceiveDump(QStringLiteral("get_config"), out);
}

bool SocketClient::fetchCapabilities(QMap<QString, QString> &out)
{
    return sendAndReceiveDump(QStringLiteral("get_capabilities"), out);
}

bool SocketClient::fetchDimensions(QMap<QString, QString> &out)
{
    return sendAndReceiveDump(QStringLiteral("get_dimensions"), out);
}

bool SocketClient::setOption(const QString &name, const QString &value, QString &error)
{
    QString cmd = QStringLiteral("set_option %1=%2").arg(name, value);
    return sendAndReceiveStatus(cmd, error);
}

bool SocketClient::sendReload(QString &error)
{
    return sendAndReceiveStatus(QStringLiteral("reload"), error);
}

bool SocketClient::sendSave(QString &error)
{
    return sendAndReceiveStatus(QStringLiteral("save"), error);
}
