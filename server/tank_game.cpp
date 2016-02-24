#include "tank_game.hpp"

#include "tank_map.hpp"

TankGame::TankGame(TankMap *tankMap, QObject *parent) : Game(parent),
    _tankMap(tankMap)
{

}

TankGame::~TankGame()
{

}

void TankGame::onPlayerMove(Client *client, int turn, QByteArray data)
{
    (void) turn;
    (void) data;
    int playerID = _playerClients.indexOf({client,true});
    Q_ASSERT(playerID != -1);

    Player * player = _players[playerID];

    if (player->movedThisTurn)
    {
        emit wantToKick(client, "several moves received in one turn");
        return;
    }

    Move move;
    //todo : parse data

    player->movedThisTurn = true;
    _currentTurnMoves.append(move);
}

void TankGame::onVisuAck(Client *client, int turn, QByteArray data)
{
    (void) turn;
    if (data.size() != 0)
        emit wantToKick(client, "invalid visualization TURN_ACK: too much data (none is allowed)");
}

void TankGame::onStart()
{
    if (_isRunning)
    {
        emit message("Cannot start game: it is already running...");
        return;
    }

    if (_playerClients.size() < 2)
    {
        emit message("Cannot start game: there must be at least 2 connected players");
        return;
    }

    if (!_tankMap->isLoaded())
    {
        emit message("Cannot start game: map is not loaded");
        return;
    }

    _tankMap->lock();

    for (GameClient c : _playerClients)
    {
        (void) c;
    }

    for (GameClient c : _visuClients)
    {
        (void) c;
    }

    _isRunning = true;
}

void TankGame::onPause()
{

}

void TankGame::onResume()
{

}

void TankGame::onStop()
{

}

void TankGame::onTurnTimerChanged(quint32 ms)
{
    (void) ms;
}

void TankGame::onTurnEnd()
{
    // todo: read move and apply them

    _currentTurnMoves.clear();
    // todo: send current turn to all clients
}

TankGame::Orientation TankGame::rotateLeft(TankGame::Orientation o)
{
    return Orientation(((quint8)o+3)%4);
}

TankGame::Orientation TankGame::rotateRight(TankGame::Orientation o)
{
    return Orientation(((quint8)o+1)%4);
}



bool TankGame::Player::tankBelongsToMe(quint32 tankID) const
{
    for (const Tank * tank : tanks)
        if (tank->tankID == tankID)
            return true;

    return false;
}
