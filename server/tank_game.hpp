#ifndef TANKGAME_HPP
#define TANKGAME_HPP

#include "game.hpp"

class TankMap;

class TankGame : public Game
{
    Q_OBJECT
public:
    enum class TankDisplacement : quint8
    {
        NONE      = 0   //! Don't move
        ,FORWARD  = 1   //! Move one cell forward
        ,BACKWARD = 2   //! Move one cell backward
    };

    enum class Rotation : quint8
    {
        NONE   = 0  //! Don't rotate
        ,LEFT  = 1  //! 90° rotation to the left
        ,RIGHT = 2  //! 90° rotation to the right
    };

    enum class Orientation : quint8
    {
        NORTH  = 0  //! Do I really need to comment this?
        ,EAST  = 1  //! Do I really need to comment this?
        ,SOUTH = 2  //! Do I really need to comment this?
        ,WEST  = 3  //! Do I really need to comment this?
    };

    static Orientation rotateLeft(Orientation o);
    static Orientation rotateRight(Orientation o);

    //! A tank can only do one of those each turn : {do a displacement, rotate, shoot}
    struct TankMove
    {
        quint32 tankID;                 //! The unique tank number
        TankDisplacement displacement;  //! The tank displacement.
        Rotation tankRotation;          //! The relative rotation of the tank.
        bool shoots;                    //! If true, the tank shoots a projectile.
        Orientation shootOrientation;   //! The direction where the fired projectile will go
        bool dropMine;                  //! If true, the tank drops a mines
    };

    struct Position
    {
        quint32 x;
        quint32 y;
    };

    struct Move
    {
        quint32 playerID;
        QVector<TankMove> tankMoves;
    };

    struct Tank
    {
        quint32 tankID;                 //! The unique tank number
        quint32 playerID;               //! The player who owns the tank
        Position position;              //! The cell in which the tank is
        Orientation tankOrientation;    //! The absolute tank orientation
        Orientation cannonOrientation;  //! The absolute cannon orientation
        quint8 invicibilityTurns;       //! The number of turns the tank will still be invincible
        quint8 health;                  //! The number of health points
        quint8 regen;                   //! The number of health point this tank gains per round
        quint8 minesCount;              //! The number of mines the tank possesses
    };

    struct Projectile
    {
        quint32 playerID;           //! The player who owns the projectile
        Orientation orientation;    //! The direction where the projectile goes
        quint8 speed;               //! The number of cells the projectile goes through each round
    };

    struct Player
    {
        quint32 score = 0;
        QVector<Tank*> tanks;
        bool movedThisTurn = false;

        bool tankBelongsToMe(quint32 tankID) const; //! Returns true if the player owns the tank. Otherwise returns false.
    };

    explicit TankGame(TankMap * tankMap, QObject *parent = 0);
    ~TankGame();

signals:

private slots:
    virtual void onPlayerMove(Client * client, int turn, QByteArray data) override;
    virtual void onVisuAck(Client * client, int turn, QByteArray data) override;

    virtual void onStart() override;
    virtual void onPause() override;
    virtual void onResume() override;
    virtual void onStop() override;
    virtual void onTurnTimerChanged(quint32 ms) override;

    //! Called at turn end
    void onTurnEnd();

private:
    QVector<Player*> _players;
    QVector<Tank*> _tanks;
    TankMap * _tankMap = nullptr;

    QList<Move> _currentTurnMoves; //! The moves that must be done on turn end
};

#endif // TANKGAME_HPP
