#pragma once

#include <QObject>
#include <QTcpServer>
#include <QMap>

class Client;

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = 0);
    void listen(quint16 port = 4242);
    ~Server();

signals:
    //! The server wants to display something
    void message(const QString &);

    void playerConnected(Client * client);
    void playerDisconnected(Client * client);
    void visuConnected(Client * client);
    void visuDisconnected(Client * client);
    void playerTurnAckReceived(Client * client, int turn, QByteArray);
    void visuTurnAckReceived(Client * client, int turn, QByteArray);

public slots:
    void onClientConnected();
    void onClientDisconnected(QTcpSocket * socket);
    void onClientWantToBeAPlayer(const QString & nick);
    void onClientWantToBeAVisu(const QString & nick);
    void onClientTurnAckReceived(int turn, QByteArray & data);

    /**
     * @brief This slot must be called when the game starts.
     * @details It allows to store that the game is currently running in order to reject new connections and to kick unlogged clients
     */
    void onGameLaunched();

    /**
     * @brief This slot must be called when the game finishes.
     * @details This allows to store that the game is not running anymore. All connected clients are logged out and must log again. New connections are allowed.
     */
    void onGameFinished();

    void kick(Client * client, const QString & reason);
    void sendWelcome(Client * client, const QByteArray & data);
    void sendGameStarts(Client * client, const QByteArray & data);
    void sendGameEnds(Client * client, const QByteArray & data);
    void sendTurn(Client * client, int turn, const QByteArray & data);

    void setServerMaxClients(quint32 maxClients);
    void setServerMaxPlayers(quint32 maxPlayers);
    void setServerMaxVisus(quint32 maxVisus);

    void listServerClients();
    void listServerPlayers();
    void listServerVisus();
    void resetClients();
    void resetAndChangeServerPort(quint16 port);

private:
    QTcpServer * _server;
    QMap<QTcpSocket*, Client*> _socketToClient;
    QSet<Client *> _clients;

    int _maxClients = 512;
    const int _maxClientID = 65535;
    int _maxPlayers = 32;
    int _maxVisus = 1;

    bool _isGameRunning = false;

    int _nbPlayers = 0;
    int _nbVisus = 0;
    quint16 _nextClientID = 0;
};
