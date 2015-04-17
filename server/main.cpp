#include <QCoreApplication>

#include "server.hpp"
#include "cli.hpp"
#include "tank_map.hpp"

int main(int argc, char **argv)
{
    QCoreApplication a(argc, argv);

    auto s = new Server();
    auto cli = new CLI();
    auto map = new TankMap();
    auto game = new TankGame(map);

    a.connect(s, SIGNAL(message(QString)), cli, SLOT(displayMessage(QString)));
    a.connect(map, SIGNAL(message(QString)), cli, SLOT(displayMessage(QString)));
    a.connect(game, SIGNAL(message(QString)), cli, SLOT(displayMessage(QString)));

    a.connect(s, SIGNAL(playerTurnAckReceived(Client*,int,QByteArray)), game, SLOT(onPlayerMove(Client*,int,QByteArray)));
    a.connect(s, SIGNAL(visuTurnAckReceived(Client*,int,QByteArray)), game, SLOT(onVisuAck(Client*,int,QByteArray)));

    a.connect(cli, SIGNAL(wantToLoadMap(QString)), map, SLOT(loadFile(QString)));

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


    map->loadFile("/home/carni/proj/tankia/maps/map2.map");

    return a.exec();
}
