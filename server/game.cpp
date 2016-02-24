#include "game.hpp"

#include "client.hpp"

Game::Game(QObject *parent) : QObject(parent)
{

}

Game::~Game()
{

}

void Game::onPlayerConnected(Client *client)
{
    if (!_isRunning)
        _playerClients.push_back({client, true});
    else
    {
        emit message("Unexpected case: the game received a new player whereas the game is running. This player is ignored");
        emit wantToKick(client, "cannot login while game is running");
    }
}

void Game::onVisuConnected(Client *client)
{
    if (!_isRunning)
        _visuClients.push_back({client, true});
    else
    {
        emit message("Unexpected case: the game received a new visu whereas the game is running. This visu is ignored");
        emit wantToKick(client, "cannot login while game is running");
    }
}

void Game::onPlayerDisconnected(Client *client)
{
    int i = _playerClients.indexOf({client,true});
    if (i != -1)
    {
        if (_isRunning)
            _playerClients[i].connected = false;
        else
            _playerClients.remove(i);
    }
}

void Game::onVisuDisconnected(Client *client)
{
    int i = _visuClients.indexOf({client,true});
    if (i != -1)
    {
        if (_isRunning)
            _visuClients[i].connected = false;
        else
            _visuClients.remove(i);
    }
}

void Game::setServer(Server *server)
{
    _server = server;
}

bool operator<(const Game::GameClient &gc1, const Game::GameClient &gc2)
{
    return gc1.client < gc2.client;
}


bool operator==(const Game::GameClient &gc1, const Game::GameClient &gc2)
{
    return gc1.client == gc2.client;
}
