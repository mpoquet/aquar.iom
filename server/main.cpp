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

    a.connect(s, SIGNAL(message(QString)), cli, SLOT(displayMessage(QString)));
    a.connect(map, SIGNAL(message(QString)), cli, SLOT(displayMessage(QString)));

    a.connect(cli, SIGNAL(wantToSetServerMaxClients(quint32)), s, SLOT(setServerMaxClients(quint32)));
    a.connect(cli, SIGNAL(wantToSetServerMaxPlayers(quint32)), s, SLOT(setServerMaxPlayers(quint32)));
    a.connect(cli, SIGNAL(wantToSetServerMaxVisus(quint32)), s, SLOT(setServerMaxVisus(quint32)));

    a.connect(cli, SIGNAL(wantToListServerClients()), s, SLOT(listServerClients()));
    a.connect(cli, SIGNAL(wantToListServerPlayers()), s, SLOT(listServerPlayers()));
    a.connect(cli, SIGNAL(wantToListServerVisus()), s, SLOT(listServerVisus()));

    a.connect(cli, SIGNAL(wantToResetClients()), s, SLOT(resetClients()));
    a.connect(cli, SIGNAL(wantToResetAndChangeServerPort(quint16)), s, SLOT(resetAndChangeServerPort(quint16)));

    map->loadFile("/home/carni/proj/tankia/maps/map2.map");

    return a.exec();
}
