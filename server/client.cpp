#include "client.hpp"

#include <QRegularExpression>

#include "server.hpp"

Client::Client(QTcpSocket *sock, int id, Server *parent) :
    QObject(parent),
    _socket(sock),
    _id(id),
    _server(parent)
{

}

void Client::updateName()
{
    if (_socket == nullptr)
        _name = "NullSocket";
    else if (_type == UNKNOWN)
        _name = _socket->peerName();
    else
        _name = QString("%1 (%2)").arg(_nick).arg(_id);
}

QByteArray Client::readNTString()
{
    for (int i = 0; i < _buffer.size(); ++i)
    {
        if (_buffer[i] == '\0')
        {
            auto res = _buffer.left(i);
            _buffer.remove(0, i+1);
            return res;
        }
    }

    return "";
}

bool Client::isNickValid(const QString &nick)
{
    QRegularExpression regex("^[\\w ]{1,16}$");
    return regex.match(nick).hasMatch();
}

void Client::onReadyRead()
{
    _buffer += _socket->read(_socket->bytesAvailable());

    switch(_type)
    {
        case UNKNOWN:
        {
            auto s = readNTString();
            if (s.startsWith("login"))
            {
                _nick = QString::fromLatin1(s.mid(5)).trimmed();
                if (isNickValid(_nick))
                {
                    qDebug() << "Valid nick: " << _nick;
                }
                else
                    kick("invalid nick: '" + _nick + "'");
            }
            else if (s.startsWith("visu"))
            {
                _nick = QString::fromLatin1(s.mid(4)).trimmed();
                if (isNickValid(_nick))
                {
                    qDebug() << "Valid nick: " << _nick;
                }
                else
                    kick("invalid nick: '" + _nick + "'");
            }
            else
                kick("invalid string received: '" + s + "'");
        } break;

        case PLAYER:
        {

        } break;

        case VISU:
        {

        } break;
    }

    if (_buffer.size() > _bufferMaxSize)
        kick("buffer size exceeded");
}

void Client::kick(const QString & reason)
{
    sendData("kicked from server: " + reason.toLatin1() + '\0');
    qDebug() << "Kicked: " << reason;
    // todo: kick
}

void Client::sendData(const QByteArray & data)
{
    _socket->write(data);
}

void Client::onDisconnected()
{
    qDebug() << "Disconnected";
}

void Client::onError(QAbstractSocket::SocketError socketError)
{
    qDebug() << "Error" << socketError;
}
