#include "touch_subscriber.h"

TouchSubscriber::TouchSubscriber(const QString &socketPath, QObject *parent)
    : QObject(parent)
    , m_socketPath(socketPath)
{
}

TouchSubscriber::~TouchSubscriber()
{
    stop();
}

void TouchSubscriber::start()
{
    if (m_socket)
        return;

    m_socket = new QLocalSocket(this);

    m_socket->connectToServer(m_socketPath);
    if (!m_socket->waitForConnected(2000)) {
        delete m_socket;
        m_socket = nullptr;
        return;
    }

    // Send subscribe command
    m_socket->write("subscribe_touches\n");
    m_socket->flush();

    // Wait for OK response (signals not connected yet to avoid race)
    if (m_socket->waitForReadyRead(2000)) {
        QByteArray resp = m_socket->readAll();
        if (!resp.contains("OK")) {
            delete m_socket;
            m_socket = nullptr;
            return;
        }
        // Any extra data after OK goes into the buffer
        int okEnd = resp.indexOf("OK\n");
        if (okEnd >= 0) {
            QByteArray extra = resp.mid(okEnd + 3);
            if (!extra.isEmpty())
                m_buffer.append(extra);
        }
    } else {
        delete m_socket;
        m_socket = nullptr;
        return;
    }

    // Now connect signals for async data reception
    connect(m_socket, &QLocalSocket::readyRead, this, &TouchSubscriber::onReadyRead);
    connect(m_socket, &QLocalSocket::disconnected, this, &TouchSubscriber::onDisconnected);

    // Process any data that came with the OK response
    if (!m_buffer.isEmpty())
        parseBuffer();
}

void TouchSubscriber::stop()
{
    if (m_socket) {
        m_socket->disconnectFromServer();
        delete m_socket;
        m_socket = nullptr;
    }
    m_buffer.clear();
}

void TouchSubscriber::onReadyRead()
{
    m_buffer.append(m_socket->readAll());
    parseBuffer();
}

void TouchSubscriber::onDisconnected()
{
    // Emit empty touches on disconnect
    emit touchesUpdated(QVector<TouchContact>());
}

void TouchSubscriber::parseBuffer()
{
    // Parse "TOUCHES <count>\n<x> <y> <z>\n...\nEND\n" messages
    while (true) {
        int endIdx = m_buffer.indexOf("END\n");
        if (endIdx < 0)
            break;

        QByteArray msg = m_buffer.left(endIdx);
        m_buffer.remove(0, endIdx + 4);

        QVector<TouchContact> contacts;
        QList<QByteArray> lines = msg.split('\n');

        for (const QByteArray &line : lines) {
            QByteArray trimmed = line.trimmed();
            if (trimmed.isEmpty())
                continue;

            if (trimmed.startsWith("TOUCHES")) {
                // Header line, skip
                continue;
            }

            // Parse "x y z"
            QList<QByteArray> parts = trimmed.split(' ');
            if (parts.size() >= 3) {
                TouchContact c;
                c.x = parts[0].toInt();
                c.y = parts[1].toInt();
                c.z = parts[2].toInt();
                contacts.append(c);
            }
        }

        emit touchesUpdated(contacts);
    }
}
