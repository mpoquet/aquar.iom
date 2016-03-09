#include <QCoreApplication>
#include <QCommandLineParser>

#include <stdio.h>

#include "server.hpp"
#include "cli.hpp"
#include "tank_map.hpp"
#include "cell_game.hpp"

int main(int argc, char **argv)
{
    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Aquar.iom server");
    parser.addHelpOption();
    parser.addOption({"map", "The map used in the game", "FILENAME", ""});
    parser.addOption({"port", "The network port used by the server", "NUMBER", "4242"});

    parser.process(a);

    QString map_filename = parser.value("map");
    QString port_string = parser.value("port");
    bool ok;
    int port = port_string.toInt(&ok);

    if (!ok)
    {
        printf("Impossible to convert port '%s' to a valid integer, aborting.\n", port_string.toStdString().c_str());
        return 1;
    }

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

    s->listen(port);
    if (!map_filename.isEmpty())
        game->load_parameters(map_filename);
    else
    {
        cli->displayMessage(QString("No map has been loaded yet.\n"
                                    "You can load one via the CLI (try 'help' to display available commands).\n"
                                    "You can also load one via command-line options (rerun server with -h)"));
    }

    return a.exec();
}
