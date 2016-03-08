#pragma once

#include <QTcpSocket>
#include <QByteArray>

class Server;
enum class Stamp : std::uint8_t;

class Client : public QObject
{
    friend class Server;
    Q_OBJECT
public:
    enum ClientType
    {
        UNKNOWN,
        PLAYER,
        VISU
    };

    Client(QTcpSocket * sock, quint16 _id, const QString & name, Server * parent);

    /**
     * @brief Validates a given nickname or not
     * @param nick The nickname to validate
     * @return true if nick is valid, false otherwise
     */
    static bool isNickValid(const QString & nick);

    ClientType type() const { return _type; }
    int id() const { return _id; }
    QString name() const { return _name; }
    QByteArray & buffer() { return _buffer; }
    const QByteArray & buffer() const { return _buffer; }

signals:
    //! The client wants to display something
    void message(const QString & msg);
    //! The client wants to become a player (it is currently UNKNOWN)
    void wantToBeAPlayer(const QString & nick);
    //! The client wants to become a visu (it is currently UNKNOWN)
    void wantToBeAVisu(const QString & nick);

    /**
     * @brief This signal is emitted when a TURN_ACK message had been received
     * @param message The complete content of the message
     */
    void messageTurnAckReceived(int turn, QByteArray & message);

    void disconnected(QTcpSocket * socket);

private slots:
    void onReadyRead();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);

    /**
     * @brief Kicks a client
     * @param reason The reason why the poor client is forced to leave
     */
    void kick(const QString & reason);

    /**
     * @brief Sets the most up-to-date TURN message that must be sent to this client.
     * @details The new message is sent directly if this client already acknowledged the last TURN message.<br/>
     * <br/>
     * Otherwise the new message is stored as the most up-to-date one (losing the previous stored message).
     * This message will be sent when the next TURN_ACK message will be received.
     * @param turn The turn the data corresponds to
     * @param data The data to send to the client
     * @pre _type in {PLAYER, VISU}
     */
    void sendTurn(quint32 turn, const QByteArray & data);

private:
    void sendStamp(const Stamp & stamp);
    void sendSizedString(const QString & s);
    void sendWelcome(const QByteArray & data);
    void sendGameStarts(const QByteArray & data);

    void sendGameEnds(const QByteArray & data);

    void internalSendTurn(quint32 turn, const QByteArray & data);

    void logout(const QString &name);
    void beAPlayer(const QString & nick);
    void beAVisu(const QString & nick);

private:
    QTcpSocket * _socket = nullptr;
    quint16 _id = 0;
    QString _nick = "Anonymous";
    ClientType _type = UNKNOWN;
    QString _name = "NullSocket";
    Server * _server = nullptr;
    QByteArray _buffer; // receive buffer
    const int _bufferMaxSize = 4096; // receive
    bool _has_been_logged_out_recently = false;

    QByteArray _sendBuffer;
    quint32 _lastTurnSent = -1;
    quint32 _lastTurnAcknowledged = -1;
    quint32 _turnToSend = -1;
};
