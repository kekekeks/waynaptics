#pragma once

#include <QObject>
#include <QLocalSocket>
#include <QVector>
#include <QPointF>

struct TouchContact {
    int x, y, z;
};

class TouchSubscriber : public QObject
{
    Q_OBJECT

public:
    explicit TouchSubscriber(const QString &socketPath, QObject *parent = nullptr);
    ~TouchSubscriber();

    void start();
    void stop();

signals:
    void touchesUpdated(const QVector<TouchContact> &contacts);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    void parseBuffer();

    QString m_socketPath;
    QLocalSocket *m_socket = nullptr;
    QByteArray m_buffer;
};
