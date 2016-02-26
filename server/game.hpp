#pragma once

#include <QObject>
#include <QTcpSocket>

class Client;
class Server;

class Game : public QObject
{
    Q_OBJECT
public:
    struct GameClient
    {
        Client * client = nullptr;
        bool connected = true;
    };

public:
    explicit Game(QObject *parent = 0);
    virtual ~Game();

    bool isRunning() const { return _isRunning; }

signals:
    //! The game wants to display something
    void message(const QString & msg);

    void gameLaunched();
    void gameFinished();

    void wantToKick(Client * client, const QString & reason);
    void wantToSendWelcome(Client * client, const QByteArray & data);
    void wantToSendGameStarts(Client * client, const QByteArray & data);
    void wantToSendGameEnds(Client * client, const QByteArray & data);
    void wantToSendTurn(Client * client, int turn, const QByteArray & data);

public slots:
    virtual void onPlayerConnected(Client * client); // if overridden, this method must be called by the child method
    virtual void onVisuConnected(Client * client); // if overridden, this method must be called by the child method
    virtual void onPlayerDisconnected(Client * client); // if overridden, this method must be called by the child method
    virtual void onVisuDisconnected(Client * client); // if overridden, this method must be called by the child method

    virtual void onPlayerMove(Client * client, int turn, QByteArray data) = 0;
    virtual void onVisuAck(Client * client, int turn, QByteArray data) = 0;

    virtual void onStart() = 0;
    virtual void onPause() = 0;
    virtual void onResume() = 0;
    virtual void onStop() = 0;
    virtual void onTurnTimerChanged(quint32 ms) = 0;

    virtual void setServer(Server * server);

protected:
    // These two attributes store players and visus. If the game is running, the size of those attributes won't change.
    QVector<GameClient> _playerClients;
    QVector<GameClient> _visuClients;

    bool _isRunning = false;
    Server * _server = nullptr;
};

bool operator<(const Game::GameClient & gc1, const Game::GameClient & gc2);
bool operator==(const Game::GameClient & gc1, const Game::GameClient & gc2);
