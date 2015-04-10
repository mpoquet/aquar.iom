#include "server.hpp"

#include <QCoreApplication>

#include "client.hpp"
#include "protocol.hpp"

Server::Server(quint16 port, QObject *parent) : QObject(parent)
{
    _server = new QTcpServer(this);
    connect(_server, SIGNAL(newConnection()), this, SLOT(onClientConnected()));

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
    auto mit = _socketToClient.find(socket);
    if (mit != _socketToClient.end())
    {
        Client * client = mit.value();
        if (client->type() == Client::PLAYER)
            emit playerDisconnected(client);
        else if (client->type() == Client::VISU)
            emit visuDisconnected(client);

        _clients.remove(client);
        _socketToClient.erase(mit);
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
    Client * c = dynamic_cast<Client *>(sender());
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

void Server::onClientTurnAckReceived(int turn, const QByteArray &data)
{
    Client * c = dynamic_cast<Client *>(sender());
    Q_ASSERT(c != nullptr);
    Q_ASSERT(c->type() == Client::PLAYER || c->type() == Client::VISU);

    if (c->type() == Client::PLAYER)
        emit playerTurnAckReceived(c, turn, data);
    else if (c->type() == Client::VISU)
        emit visuTurnAckReceived(c, turn, data);
}

void Server::onGameLaunched()
{
    Q_ASSERT(!_isGameRunning);
    _isGameRunning = true;

    for (auto c : _clients)
        if (c->type() == Client::UNKNOWN)
            c->kick("game was launched before you logged in");
}

void Server::onGameFinished()
{
    Q_ASSERT(_isGameRunning);
    _isGameRunning = false;

    for (Client * c : _clients)
    {
        c->logout();

        if (c->type() == Client::PLAYER)
            emit playerDisconnected(c);
        else if (c->type() == Client::VISU)
            emit visuDisconnected(c);
    }

    _clients.clear();
    _socketToClient.clear();
}

void Server::kick(Client *client, const QString &reason)
{
    if (_clients.contains(client))
        client->kick(reason);
}

void Server::sendWelcome(Client *client, const QByteArray &data)
{
    if (_clients.contains(client))
        client->sendWelcome(data);
}

void Server::sendGameStarts(Client *client, const QByteArray &data)
{
    if (_clients.contains(client))
        client->sendGameStarts(data);
}

void Server::sendGameEnds(Client *client, const QByteArray &data)
{
    if (_clients.contains(client))
        client->sendGameEnds(data);
}

void Server::sendTurn(Client *client, const QByteArray &data)
{
    if (_clients.contains(client))
        client->sendGameEnds(data);
}

void Server::onClientConnected()
{
    Q_ASSERT(_nextClientID+1 > _nextClientID);

    auto sock = _server->nextPendingConnection();
    QString cname = QString("(%1:%2)").arg(sock->peerAddress().toString()).arg(sock->peerPort());

    auto c = new Client(sock, _nextClientID++, cname, this);
    emit message(QString("New client %1 connected").arg(cname));

    connect(sock, SIGNAL(disconnected()), c, SLOT(onDisconnected()));
    connect(sock, SIGNAL(error(QAbstractSocket::SocketError)), c, SLOT(onError(QAbstractSocket::SocketError)));
    connect(sock, SIGNAL(readyRead()), c, SLOT(onReadyRead()));

    connect(c, SIGNAL(message(QString)), this, SIGNAL(message(QString)));
    connect(c, SIGNAL(disconnected(QTcpSocket*)), this, SLOT(onClientDisconnected(QTcpSocket*)));
    connect(c, SIGNAL(wantToBeAPlayer(QString)), this, SLOT(onClientWantToBeAPlayer(QString)));
    connect(c, SIGNAL(wantToBeAVisu(QString)), this, SLOT(onClientWantToBeAVisu(QString)));
    connect(c, SIGNAL(messageTurnAckReceived(int,QByteArray)), this, SLOT(onClientTurnAckReceived(int,QByteArray)));

    if (!_isGameRunning)
    {
        if (_clients.size() < _maxClients)
        {
            _clients.insert(c);
            _socketToClient[sock] = c;
        }
        else
            c->kick("maximum number of simultaneous connections reached");
    }
    else
        c->kick("game is running");
}
