#pragma once

#include <QTcpSocket>
#include <QByteArray>

class Server;

class Client : public QObject
{
    Q_OBJECT
public:
    enum ClientType
    {
        UNKNOWN,
        PLAYER,
        VISU
    };

    Client(QTcpSocket * sock, int _id, Server * parent);
    void updateName();
    QByteArray readNTString();
    static bool isNickValid(const QString & nick);

public slots:
    void onReadyRead();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);

    void kick(const QString & reason);
    void sendData(const QByteArray & data);

public:
    QTcpSocket * _socket = nullptr;
    int _id = 0;
    QString _nick = "Anonymous";
    ClientType _type = UNKNOWN;
    QString _name = "NullSocket";
    Server * _server = nullptr;
    QByteArray _buffer;
    const int _bufferMaxSize = 1024;
};
