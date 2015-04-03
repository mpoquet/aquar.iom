#include "server.hpp"

#include <QCoreApplication>

#include "client.hpp"
#include "protocol.hpp"

Server::Server(quint16 port, QObject *parent) : QObject(parent)
{
    _server = new QTcpServer(this);
    connect(_server, SIGNAL(newConnection()), this, SLOT(onNewConnection()));

    if (!_server->listen(QHostAddress::Any, port))
    {
        emit message(QString("Cannot listen port %1").arg(port));
        qApp->exit(1);
    }
    else
        emit message(QString("Listening port %1").arg(port));
}

Server::~Server()
{

}

void Server::onClientDisconnected(QTcpSocket *socket)
{
    auto mit = _clients.find(socket);
    if (mit != _clients.end())
    {
        Client * client = mit.value();
        if (client->type() == Client::PLAYER)
            emit playerDisconnected(client);
        else if (client->type() == Client::VISU)
            emit visuDisconnected(client);
    }
}

void Server::onClientWantToBeAPlayer(const QString &nick)
{
    Client * c = dynamic_cast<Client *>(sender());
    Q_ASSERT(c != nullptr);
    Q_ASSERT(c->type() == Client::UNKNOWN);

    if (!_isGameRunning)
    {
        if (_nbPlayers < _maxPlayers)
        {
            c->beAPlayer(nick);
            _nbPlayers++;
            emit playerConnected(c);
        }
        else
            c->kick("maximum number of players reached");
    }
    else
        c->kick("cannot login while game is running");
}

void Server::onClientWantToBeAVisu(const QString &nick)
{
    Client * c = (Client *)sender();
    Q_ASSERT(c != nullptr);
    Q_ASSERT(c->type() == Client::UNKNOWN);

    if (!_isGameRunning)
    {
        if (_nbVisus < _maxVisus)
        {
            c->beAVisu(nick);
            _nbVisus++;
            emit visuConnected(c);
        }
        else
            c->kick("maximum number of visus reached");
    }
    else
        c->kick("cannot login while game is running");
}

void Server::onGameLaunched()
{
    Q_ASSERT(!_isGameRunning);
    _isGameRunning = true;

    for (auto key: _clients.keys())
    {
        Client * c = _clients[key];
        if (c->type() == Client::UNKNOWN)
            c->kick("not logged yet whereas game is launched");
    }
}

void Server::onGameFinished()
{
    Q_ASSERT(_isGameRunning);
    _isGameRunning = false;

    for (auto key: _clients.keys())
    {
        Client * c = _clients[key];
        if (c->type() == Client::PLAYER)
            c->beAnUnknown();
    }
}

void Server::onNewConnection()
{
    Q_ASSERT(_nextClientID+1 > _nextClientID);

    auto sock = _server->nextPendingConnection();
    QString cname = QString("(%1:%2)").arg(sock->peerAddress().toString()).arg(sock->peerPort());

    auto c = new Client(sock, _nextClientID++, cname, this);
    emit message(QString("New client %1 connected").arg(cname));

    connect(sock, SIGNAL(disconnected()), c, SLOT(onDisconnected()));
    connect(sock, SIGNAL(error(QAbstractSocket::SocketError)), c,    SLOT(onError(QAbstractSocket::SocketError)));
    connect(sock, SIGNAL(readyRead()), c, SLOT(onReadyRead()));
    connect(c, SIGNAL(message(QString)), this, SIGNAL(message(QString)));
    connect(c, SIGNAL(wantToBeAPlayer(QString)), this, SLOT(onClientWantToBeAPlayer(QString)));
    connect(c, SIGNAL(wantToBeAVisu(QString)), this, SLOT(onClientWantToBeAVisu(QString)));

    if (!_isGameRunning)
    {
        if (_clients.size() < _maxClients)
            _clients[sock] = c;
        else
            c->kick("maximum number of simultaneous connections reached");
    }
    else
        c->kick("game is running");
}
