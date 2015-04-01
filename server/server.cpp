#include "server.hpp"

#include <QCoreApplication>

#include "client.hpp"

Server::Server(quint16 port, QObject *parent) : QObject(parent)
{
    _server = new QTcpServer(this);
    connect(_server, &QTcpServer::newConnection, this, &Server::onNewConnection);

    if (!_server->listen(QHostAddress::Any, port))
    {
        qDebug() << "Cannot listen";
        qApp->exit(1);
    }
    qDebug() << "Server listening port" << port;
}

Server::~Server()
{

}

void Server::onNewConnection()
{
    auto sock = _server->nextPendingConnection();
    qDebug() << "New connection";

    if (_clients.size() < _maxClients)
    {
        auto c = new Client(sock, _nextClientID++, this);
        _clients[sock] = c;

        connect(sock, &QAbstractSocket::disconnected, c, &Client::onDisconnected);
        connect(sock, SIGNAL(error(QAbstractSocket::SocketError)), c, SLOT(onError(QAbstractSocket::SocketError)));
        connect(sock, &QAbstractSocket::readyRead,    c, &Client::onReadyRead);
    }
}
