#pragma once

#include <QObject>
#include <QTcpServer>
#include <QMap>

class Client;

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(quint16 port = 4242, QObject *parent = 0);
    ~Server();

signals:

public slots:

private slots:
    void onNewConnection();

private:
    QTcpServer * _server;
    QMap<QTcpSocket*, Client*> _clients;
    const int _maxClients = 512;
    unsigned int _nextClientID = 0;
};
