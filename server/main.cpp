#include <QCoreApplication>

#include "server.hpp"
#include "logger.hpp"
#include "tank_map.hpp"

int main(int argc, char **argv)
{
    QCoreApplication a(argc, argv);

    auto s = new Server();
    auto l = new Logger();
    auto map = new TankMap();

    a.connect(s, SIGNAL(message(QString)), l, SLOT(handleMessage(QString)));
    a.connect(map, SIGNAL(message(QString)), l, SLOT(handleMessage(QString)));

    map->loadFile("/home/carni/proj/tankia/maps/map1.map");

    return a.exec();
}
