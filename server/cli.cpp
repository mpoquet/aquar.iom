#include "cli.hpp"

#include <stdio.h>
#include <iostream>

CLI::CLI(QObject *parent) : QObject(parent)
{
    _inNotifier = new QSocketNotifier(fileno(stdin), QSocketNotifier::Read, this);
    connect(_inNotifier, SIGNAL(activated(int)), this, SLOT(onCommandEntered(int)));

    _outStream = new QTextStream(stdout, QIODevice::WriteOnly);

    _help = "list of commands:\n"
            "  help       displays this help\n"
            "  load MAP   loads a map\n"
            "  timer MS   sets the game turn time\n"
            "\n"
            "  max NB     sets the maximum number of clients\n"
            "  maxp NB    sets the maximum number of players\n"
            "  maxv NB    sets the maximum number of visualizations\n"
            "\n"
            "  list       lists connected clients\n"
            "  listp      lists connected players\n"
            "  listv      lists connected visualizations\n"
            "  reset      closes all client sockets\n"
            "  reset PORT closes all client sockets, closes the server socket,\n"
            "             reopens it and listens to the given port\n"
            "\n"
            "  start      starts the game\n"
            "  pause      pauses the game\n"
            "  resume     unpauses the game\n"
            "  stop       stops the game\n";

    _invalidCommand = ": invalid command. Type 'help' to list available commands.";
}

void CLI::displayMessage(const QString &message)
{
    *_outStream << message << endl;
    _outStream->flush();
}

void CLI::onCommandEntered(int socket)
{
    (void) socket;

    std::string line;
    std::getline(std::cin, line);

    QString qline = QString::fromLatin1(line.c_str()).trimmed();

    if (qline.isEmpty())
        return;

    QStringList words = qline.split(" ");

    if (words.size() == 1)
    {
        QString w = words[0];

        if (w == "help")
            displayMessage(_help);
        else if (w == "list")
            emit wantToListServerClients();
        else if (w == "listp")
            emit wantToListServerPlayers();
        else if (w == "listv")
            emit wantToListServerVisus();
        else if (w == "reset")
            emit wantToResetClients();
        else if (w == "start")
            emit wantToStartGame();
        else if (w == "pause")
            emit wantToPauseGame();
        else if (w == "resume")
            emit wantToResumeGame();
        else if (w == "stop")
            emit wantToStopGame();
        else
            displayMessage(QString("'%1': %2").arg(qline, _invalidCommand));
    }
    else if (words.size() == 2)
    {
        QString w = words[0];
        QString arg = words[1];
        bool okUI;
        quint32 argUI = arg.toUInt(&okUI);

        if (w == "load")
            emit wantToLoadMap(arg);
        else if (okUI)
        {
            if (w == "timer")
                emit wantToSetGameTurnTimer(argUI);
            else if (w == "max")
                emit wantToSetServerMaxClients(argUI);
            else if (w == "maxp")
                emit wantToSetServerMaxPlayers(argUI);
            else if (w == "maxv")
                emit wantToSetServerMaxVisus(argUI);
            else if (w == "reset")
                emit wantToResetAndChangeServerPort(argUI);
            else
                displayMessage(QString("'%1': %2").arg(qline, _invalidCommand));
        }
        else
            displayMessage(QString("'%1': %2").arg(qline, _invalidCommand));
    }
    else
        displayMessage(QString("'%1': %2").arg(qline, _invalidCommand));
}
