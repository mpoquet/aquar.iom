#pragma once

#include <QObject>
#include <QTcpSocket>

class Client;

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

    bool canBeStarted() const;
    bool isRunning() const { return _isRunning; }

signals:
    //! The game wants to display something
    void message(const QString & msg);

    //todo: create according slots in Server
    void wantToKick(Client * client, const QString & reason);
    void wantToSendWelcome(Client * client, const QByteArray & data);
    void wantToSendGameStarts(Client * client, const QByteArray & data);
    void wantToSendGameEnds(Client * client, const QByteArray & data);
    void wantToSendTurn(Client * client, const QByteArray & data);

public slots:
    virtual void onPlayerConnected(Client * client);
    virtual void onVisuConnected(Client * client);
    virtual void onPlayerDisonnected(Client * client);
    virtual void onVisuDisconnected(Client * client);

    virtual void onPlayerMove(const QByteArray & data) = 0;
    virtual void onVisuAck(const QByteArray & data) = 0;
    virtual void startGame() = 0;

protected:
    // These two attributes store players and visus. If the game is running, the size of those attributes won't change.
    QVector<GameClient> _players;
    QVector<GameClient> _visus;

    bool _isRunning = false;
};

bool operator<(const Game::GameClient & gc1, const Game::GameClient & gc2);
bool operator==(const Game::GameClient & gc1, const Game::GameClient & gc2);
