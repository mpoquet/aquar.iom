#pragma once

#include <QTcpSocket>
#include <QByteArray>

class Server;
enum class Stamp : std::uint8_t;

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

    Client(QTcpSocket * sock, quint16 _id, const QString & name, Server * parent);
    /**
     * @brief Reads a null-terminated latin1 string from the current buffer
     * @param clearBuffer If true, the bytes read (and the '\0') are removed from the buffer. If false or if there is no '\0' in the buffer, the buffer remains the same.
     * @return The string (as a QByteArray which does NOT contain '\0') if '\0' was found on the buffer, "" otherwise
     */
    QByteArray readNTString(bool clearBuffer, int beginPos = 0);

    /**
     * @brief Validates a given nickname or not
     * @param nick The nickname to validate
     * @return true if nick is valid, false otherwise
     */
    static bool isNickValid(const QString & nick);

    ClientType type() const { return _type; }
    int id() const { return _id; }
    QByteArray & buffer() { return _buffer; }
    const QByteArray & buffer() const { return _buffer; }

signals:
    //! The client wants to display something
    void message(const QString & msg);
    //! The client want to become a player (it is currently UNKNOWN)
    void wantToBeAPlayer(const QString & nick);
    //! The client want to become a visu (it is currently UNKNOWN)
    void wantToBeAVisu(const QString & nick);

    /**
     * @brief This signal is emitted when a TURN_ACK message had been received
     * @param message The complete content of the message
     */
    void messageTurnAckReceived(const QByteArray & message);

private slots:
    void onReadyRead();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);

public slots:
    void beAnUnknown();
    void beAPlayer(const QString & nick);
    void beAVisu(const QString & nick);

    /**
     * @brief Kicks a client
     * @param reason The reason why the poor client is forced to leave
     */
    void kick(const QString & reason);

    /**
     * @brief Sets the most TURN message that must be sent to this client.
     * @details The new message is sent directly if this client already acknowledged the last TURN message.<br/>
     * <br/>
     * Otherwise the new message is stored as the most up-to-date one (losing the previous stored message).
     * This message will be sent when the next TURN_ACK message will be received.
     * @param turn The turn the data corresponds to
     * @param data The data to send to the client
     * @pre _type in {PLAYER, VISU}
     */
    void setUpToDateTurnMessage(quint32 turn, const QByteArray & data); //todo: rename this method

private:
    void sendStamp(const Stamp & stamp);
    void sendSizedString(const QString & s);
    void sendTurnMessage(quint32 turn, const QByteArray & data);

public:
    QTcpSocket * _socket = nullptr;
    quint16 _id = 0;
    QString _nick = "Anonymous";
    ClientType _type = UNKNOWN;
    QString _name = "NullSocket";
    Server * _server = nullptr;
    QByteArray _buffer; // receive buffer
    const int _bufferMaxSize = 4096; // receive

    QByteArray _sendBuffer;
    quint32 _lastTurnSent = -1;
    quint32 _lastTurnAcknowledged = -1;
    quint32 _turnToSend = -1;
};
