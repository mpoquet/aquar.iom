#include "client.hpp"

#include <QRegularExpression>
#include <QtEndian>

#include "server.hpp"
#include "logger.hpp"
#include "protocol.hpp"

Client::Client(QTcpSocket *sock, quint16 id, const QString &name, Server *parent) :
    QObject(parent),
    _socket(sock),
    _id(id),
    _name(name),
    _server(parent)
{
}

QByteArray Client::readNTString(bool clearBuffer, int beginPos)
{
    for (int i = beginPos; i < _buffer.size(); ++i)
    {
        if (_buffer[i] == '\0')
        {
            auto res = _buffer.mid(beginPos, i);

            if (clearBuffer)
                _buffer.remove(beginPos, i+1);

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
    qDebug() << "buffer length before read: " << _buffer.size();
    _buffer += _socket->read(_socket->bytesAvailable());
    qDebug() << "received " << _buffer.size() << " bytes";

    if (_buffer.size() > _bufferMaxSize)
    {
        kick(QString("buffer size exceeded (currentSize=%1, maxSize=%2)").arg(_buffer.size()).arg(_bufferMaxSize));
        return;
    }

    int bytesRead = 0;

    // If the message stamp cannot be read
    if (_buffer.size() < bytesRead + 1)
        return;

    Stamp stamp = (Stamp)((quint8)_buffer[0]);
    bytesRead += 1;

    // If the client is not logged yet
    if (_type == UNKNOWN)
    {
        if (stamp == Stamp::LOGIN_PLAYER || stamp == Stamp::LOGIN_VISU)
        {
            // If the string size cannot be read
            if (_buffer.size() < bytesRead + 4)
                return;

            int stringSize = *((quint32*)(_buffer.data()+bytesRead));
            bytesRead += 4;
            stringSize = qFromLittleEndian(stringSize);

            if (stringSize >= _bufferMaxSize)
            {
                kick(QString("string too long (receivedLength=%1, maxLength=%2)").arg(stringSize).arg(_bufferMaxSize));
                return;
            }

            // If the string cannot be read
            if (_buffer.size() < bytesRead + stringSize)
                return;

            QString nick = QString::fromLatin1(_buffer.mid(bytesRead, stringSize)).trimmed();
            bytesRead += stringSize;

            if (isNickValid(nick))
            {
                _buffer = _buffer.mid(bytesRead);

                if (stamp == Stamp::LOGIN_PLAYER)
                    emit wantToBeAPlayer(nick);
                else
                    emit wantToBeAVisu(nick);
            }
            else
                kick(QString("invalid nick ('%1')").arg(nick));
        }
        else
            kick(QString("invalid message type received (stamp=%1)").arg((quint32)stamp));
    }
    // Otherwise, the client is a PLAYER or a VISU
    else
    {
        if (stamp == Stamp::TURN_ACK)
        {
            // If the message size cannot be read
            if (_buffer.size() < bytesRead + 4)
                return;

            int messageSize = *((quint32*)(_buffer.data()+bytesRead));
            bytesRead += 4;
            messageSize = qFromLittleEndian(messageSize);

            // If the message cannot be read entirely
            if (_buffer.size() < bytesRead + messageSize)
                return;

            int turn = *((quint32*)(_buffer.data()+bytesRead));
            bytesRead += 4;
            turn = qFromLittleEndian(turn);

            QByteArray message = _buffer.mid(bytesRead, messageSize - 4);
            bytesRead += messageSize;

            _buffer = _buffer.mid(bytesRead);

            qDebug() << "TurnAck message of length" << message.size() << ", content=" << message;
            _lastTurnAcknowledged = turn;
            emit messageTurnAckReceived(message);

            // Now that the TURN_ACK had been received, let's send the most up-to-date TURN message if needed
            if (_turnToSend > _lastTurnAcknowledged)
                sendTurnMessage(_turnToSend, _sendBuffer);
        }
        else
            kick(QString("invalid message type received (stamp=%1)").arg((quint32)stamp));
    }
}

void Client::kick(const QString & reason)
{
    sendSizedString("kicked: " + reason);
    emit message(QString("Client %1 kicked from server: %2").arg(_name, reason));
    _socket->close();
}

void Client::setUpToDateTurnMessage(quint32 turn, const QByteArray & data)
{
    Q_ASSERT(_type == PLAYER || _type == VISU);
    Q_ASSERT(turn + 1 > turn);

    // If the client did not acknowledged the last message
    if (_lastTurnAcknowledged != _lastTurnSent)
    {
        _sendBuffer = data;
        _turnToSend = turn;
    }
    else
        sendTurnMessage(turn, data);
}

void Client::sendStamp(const Stamp & stamp)
{
    const quint8 value = (quint8) stamp;
    _socket->write((const char*)&value, 1);
}

void Client::sendSizedString(const QString &s)
{
    QByteArray qba = s.toLatin1();
    const quint32 size = qba.size();

    _socket->write((const char *)&size, 4);
    _socket->write(qba);
}

void Client::sendTurnMessage(quint32 turn, const QByteArray &data)
{
    const quint32 msgSize = data.size() + 4;

    sendStamp(Stamp::TURN);
    _socket->write((const char*)&msgSize, 4);
    _socket->write((const char*)&turn, 4);
    _socket->write(data);

    _lastTurnSent = turn;
}

void Client::onDisconnected()
{
    emit message(QString("Client %1 disconnected").arg(_name));
}

void Client::onError(QAbstractSocket::SocketError socketError)
{
    if (socketError != QAbstractSocket::RemoteHostClosedError)
        emit message(QString("Client %1 error:%1").arg(_name, socketError));
}

void Client::beAnUnknown()
{
    _type = UNKNOWN;
    _nick = "Anonymous";

    sendStamp(Stamp::LOGOUT);
}

void Client::beAPlayer(const QString &nick)
{
    _type = PLAYER;
    _nick = nick;

    auto newName = QString("%1 (%2)").arg(_nick).arg(_id);
    emit message(QString("New player: %1 -> %2").arg(_name, newName));
    _name = newName;

    sendStamp(Stamp::LOGIN_ACK);
}

void Client::beAVisu(const QString &nick)
{
    _type = VISU;
    _nick = nick;

    auto newName = QString("%1 (%2)").arg(_nick).arg(_id);
    emit message(QString("New visu: %1 -> %2").arg(_name, newName));
    _name = newName;

    sendStamp(Stamp::LOGIN_ACK);
}
