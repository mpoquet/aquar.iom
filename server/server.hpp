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
    //! The server wants to display something
    void message(const QString &);

    void playerConnected(Client * client);
    void playerDisconnected(Client * client);
    void visuConnected(Client * client);
    void visuDisconnected(Client * client);

private slots:
    void onNewConnection();

    void onClientDisconnected(QTcpSocket * socket);
    void onClientWantToBeAPlayer(const QString & nick);
    void onClientWantToBeAVisu(const QString & nick);

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

private:
    QTcpServer * _server;
    QMap<QTcpSocket*, Client*> _clients;

    const int _maxClients = 512;
    const int _maxClientID = 65535;
    const int _maxPlayers = 32;
    const int _maxVisus = 1;

    bool _isGameRunning = false;

    int _nbPlayers = 0;
    int _nbVisus = 0;
    quint16 _nextClientID = 0;
};
