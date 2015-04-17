#pragma once

#include <QObject>
#include <QIODevice>
#include <QSocketNotifier>
#include <QTextStream>

class CLI : public QObject
{
    Q_OBJECT
public:
    CLI(QObject *parent = nullptr);

signals:
    void wantToLoadMap(const QString & filename);

    void wantToSetServerMaxClients(quint32 maxClients);
    void wantToSetServerMaxPlayers(quint32 maxPlayers);
    void wantToSetServerMaxVisus(quint32 maxVisus);
    void wantToListServerClients();
    void wantToListServerPlayers();
    void wantToListServerVisus();
    void wantToResetClients();
    void wantToResetAndChangeServerPort(quint16 port);

    void wantToStartGame();
    void wantToPauseGame();
    void wantToResumeGame();
    void wantToStopGame();
    void wantToSetGameTurnTimer(quint32 ms);

public slots:
    void displayMessage(const QString & message);
    void onCommandEntered(int socket);

private:
    QSocketNotifier * _inNotifier;

    QTextStream * _outStream;

    QString _help;
    QString _invalidCommand;
};
