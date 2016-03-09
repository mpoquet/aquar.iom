#include "server.hpp"

#include <QCoreApplication>

#include "client.hpp"
#include "protocol.hpp"

Server::Server(QObject *parent) : QObject(parent)
{
    _server = new QTcpServer(this);
    connect(_server, SIGNAL(newConnection()), this, SLOT(onClientConnected()));
}

void Server::listen(quint16 port)
{
    if (!_server->listen(QHostAddress::Any, port))
        emit message(QString("Cannot listen port %1").arg(port));
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
        {
            _nbPlayers--;
            emit playerDisconnected(client);
        }
        else if (client->type() == Client::VISU)
        {
            _nbVisus--;
            emit visuDisconnected(client);
        }
        else
        {
            emit message("Unlogged client disconnected");
        }

        _clients.remove(client);
        _socketToClient.erase(mit);

        socket->deleteLater();
    }
    else
    {
        //emit message(QString("Unknown client disconnected"));
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

void Server::onClientTurnAckReceived(int turn, QByteArray &data)
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
    emit message("Server onGameLaunched");
    _isGameRunning = true;

    for (auto c : _clients)
        if (c->type() == Client::UNKNOWN)
            c->kick("game was launched before you logged in");
}

void Server::onGameFinished()
{
    Q_ASSERT(_isGameRunning);
    emit message("Server onGameFinished");
    _isGameRunning = false;

    for (Client * c : _clients)
    {
        QTcpSocket * sock = c->_socket;
        QString cname = QString("(%1:%2)").arg(sock->peerAddress().toString()).arg(sock->peerPort());
        c->logout(cname);

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

void Server::sendTurn(Client *client, int turn, const QByteArray &data)
{
    if (_clients.contains(client))
        client->sendTurn(turn, data);
}

void Server::setServerMaxClients(quint32 maxClients)
{
    _maxClients = maxClients;

    if (_clients.size() <= _maxClients)
        emit message(QString("Maximum number of clients set to %1").arg(_maxClients));
    else
        emit message(QString("Maximum number of clients set to %1. There are however %2 currently connected clients").arg(_maxClients).arg(_clients.size()));
}

void Server::setServerMaxPlayers(quint32 maxPlayers)
{
    _maxPlayers = maxPlayers;

    if (_nbPlayers <= _maxPlayers)
        emit message(QString("Maximum number of players set to %1").arg(_maxPlayers));
    else
        emit message(QString("Maximum number of players set to %1. There are however %2 currently connected players").arg(_maxPlayers).arg(_nbPlayers));
}

void Server::setServerMaxVisus(quint32 maxVisus)
{
    _maxVisus = maxVisus;

    if (_nbVisus <= _maxVisus)
        emit message(QString("Maximum number of visus set to %1").arg(_maxVisus));
    else
        emit message(QString("Maximum number of visus set to %1. There are however %2 currently connected visus").arg(_maxVisus).arg(_nbVisus));
}

void Server::listServerClients()
{
    QString resu = "Unlogged clients:\n";
    QString resp = "Players:\n";
    QString resv = "Visus:\n";

    int nbu = 0;
    int nbp = 0;
    int nbv = 0;

    for (Client * client : _clients)
    {
        if (client->_type == Client::ClientType::UNKNOWN)
        {
            resu += QString("  (%1:%2)\n").arg(client->_socket->peerAddress().toString()).arg(client->_socket->localPort());
            ++nbu;
        }
        else if (client->_type == Client::ClientType::PLAYER)
        {
            resp += QString("  %3 (%1:%2)\n").arg(client->_socket->peerAddress().toString()).arg(client->_socket->localPort()).arg(client->_nick);
            ++nbp;
        }
        else
        {
            resv += QString("  %3 (%1:%2)\n").arg(client->_socket->peerAddress().toString()).arg(client->_socket->localPort()).arg(client->_nick);
            ++nbv;
        }
    }

    if ((nbu == 0) && (nbp == 0) && (nbv == 0))
        emit message("No client connected.");
    else
    {
        QString s;
        if (nbu > 0)
            s += resu;
        if (nbp > 0)
            s += resp;
        if (nbv > 0)
            s += resv;
        emit message(s);
    }
}

void Server::listServerPlayers()
{
    int nbp = 0;
    QString resp = "Players:\n";

    for (Client * client : _clients)
    {
        if (client->_type == Client::ClientType::PLAYER)
        {
            resp += QString("  %3 (%1:%2)\n").arg(client->_socket->peerAddress().toString()).arg(client->_socket->localPort()).arg(client->_nick);
            ++nbp;
        }
    }

    if (nbp > 0)
        emit message(resp);
    else
        emit message("No player connected.");
}

void Server::listServerVisus()
{
    int nbv = 0;
    QString resv = "Visus:\n";

    for (Client * client : _clients)
    {
        if (client->_type == Client::ClientType::VISU)
        {
            resv += QString("%3 (%1:%2)\n").arg(client->_socket->peerAddress().toString()).arg(client->_socket->localPort()).arg(client->_nick);
            ++nbv;
        }
    }

    if (nbv > 0)
        emit message(resv);
    else
        emit message("No visu connected.");
}

void Server::resetClients()
{
    for (Client * client : _clients)
        client->kick("reset from console");
}

void Server::resetAndChangeServerPort(quint16 port)
{
    for (Client * client : _clients)
        client->kick("reset from console");

    _server->close();
    _server->listen(QHostAddress::Any, port);

    emit message(QString("Server listening to port %1").arg(port));
}

void Server::onClientConnected()
{
    Q_ASSERT(_nextClientID+1 > _nextClientID);

    auto sock = _server->nextPendingConnection();
    QString cname = QString("(%1:%2)").arg(sock->peerAddress().toString()).arg(sock->peerPort());

    auto c = new Client(sock, _nextClientID++, cname, this);
    emit message(QString("New client: %1").arg(cname));

    connect(sock, SIGNAL(disconnected()), c, SLOT(onDisconnected()));
    connect(sock, SIGNAL(error(QAbstractSocket::SocketError)), c, SLOT(onError(QAbstractSocket::SocketError)));
    connect(sock, SIGNAL(readyRead()), c, SLOT(onReadyRead()));

    connect(c, SIGNAL(message(QString)), this, SIGNAL(message(QString)));
    connect(c, SIGNAL(disconnected(QTcpSocket*)), this, SLOT(onClientDisconnected(QTcpSocket*)));
    connect(c, SIGNAL(wantToBeAPlayer(QString)), this, SLOT(onClientWantToBeAPlayer(QString)));
    connect(c, SIGNAL(wantToBeAVisu(QString)), this, SLOT(onClientWantToBeAVisu(QString)));
    connect(c, SIGNAL(messageTurnAckReceived(int,QByteArray &)), this, SLOT(onClientTurnAckReceived(int,QByteArray&)));

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
