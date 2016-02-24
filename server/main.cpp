#include <QCoreApplication>

#include "server.hpp"
#include "cli.hpp"
#include "tank_map.hpp"
#include "cell_game.hpp"

int main(int argc, char **argv)
{
    QCoreApplication a(argc, argv);

    auto s = new Server();
    auto cli = new CLI();
    auto game = new CellGame();
    game->setServer(s);

    a.connect(s, SIGNAL(message(QString)), cli, SLOT(displayMessage(QString)));
    a.connect(game, SIGNAL(message(QString)), cli, SLOT(displayMessage(QString)));

    a.connect(s, SIGNAL(playerConnected(Client*)), game, SLOT(onPlayerConnected(Client*)));
    a.connect(s, SIGNAL(playerDisconnected(Client*)), game, SLOT(onPlayerDisconnected(Client*)));
    a.connect(s, SIGNAL(visuConnected(Client*)), game, SLOT(onVisuConnected(Client*)));
    a.connect(s, SIGNAL(visuDisconnected(Client*)), game, SLOT(onVisuDisconnected(Client*)));
    a.connect(s, SIGNAL(playerTurnAckReceived(Client*,int,QByteArray)), game, SLOT(onPlayerMove(Client*,int,QByteArray)));
    a.connect(s, SIGNAL(visuTurnAckReceived(Client*,int,QByteArray)), game, SLOT(onVisuAck(Client*,int,QByteArray)));

    a.connect(cli, SIGNAL(wantToLoadMap(QString)), game,SLOT(load_parameters(QString)));

    a.connect(cli, SIGNAL(wantToSetServerMaxClients(quint32)), s, SLOT(setServerMaxClients(quint32)));
    a.connect(cli, SIGNAL(wantToSetServerMaxPlayers(quint32)), s, SLOT(setServerMaxPlayers(quint32)));
    a.connect(cli, SIGNAL(wantToSetServerMaxVisus(quint32)), s, SLOT(setServerMaxVisus(quint32)));
    a.connect(cli, SIGNAL(wantToListServerClients()), s, SLOT(listServerClients()));
    a.connect(cli, SIGNAL(wantToListServerPlayers()), s, SLOT(listServerPlayers()));
    a.connect(cli, SIGNAL(wantToListServerVisus()), s, SLOT(listServerVisus()));
    a.connect(cli, SIGNAL(wantToResetClients()), s, SLOT(resetClients()));
    a.connect(cli, SIGNAL(wantToResetAndChangeServerPort(quint16)), s, SLOT(resetAndChangeServerPort(quint16)));

    a.connect(cli, SIGNAL(wantToStartGame()), game, SLOT(onStart()));
    a.connect(cli, SIGNAL(wantToPauseGame()), game, SLOT(onPause()));
    a.connect(cli, SIGNAL(wantToResumeGame()), game, SLOT(onResume()));
    a.connect(cli, SIGNAL(wantToStopGame()), game, SLOT(onStop()));
    a.connect(cli, SIGNAL(wantToSetGameTurnTimer(quint32)), game, SLOT(onTurnTimerChanged(quint32)));

    game->load_parameters("../maps/map2p.json");

    return a.exec();
}
