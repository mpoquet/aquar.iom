#ifndef TANKMAP_HPP
#define TANKMAP_HPP

#include <QObject>
#include <QVector>
#include <QMap>
#include <QDebug>

#include "tank_game.hpp"

class TankMap : public QObject
{
    Q_OBJECT
public:
    struct Position
    {
        quint32 x;
        quint32 y;
    };

    //! All cells are crossable but walls. The bonuses are spawners.
    enum class Cell : char
    {
        EMPTY             = ' ' //! You can move on this, projectile goes through it...
        ,UNBREAKABLE_WALL = 'X' //! A wall that cannot be destroyed
        ,BREAKABLE_WALL   = 'x' //! A wall that can be destroyed
        ,BONUS_POINTS     = 'p' //! Adds points to the player who picks it
        ,BONUS_MINES      = 'm' //! Adds mines to the tank that picks it
        ,BONUS_DAMAGE     = 'd' //! Double damage
        ,BONUS_BIGSHOT    = 'b' //! The shoot only dies on WALL and other BIGSHOTS
        ,TANK_SPAWN       = 's' //! Tanks respawn on these
    };

    const Cell & at(const Position & p) const;
    Cell & at(const Position & p);

    const Cell & at(quint32 x, quint32 y) const;
    Cell & at(quint32 x, quint32 y);

    bool isInMap(const Position & p) const;
    bool isInMap(quint32 x, quint32 y) const;

    Position spawnPositionOfTank(int tankID) const;
    TankGame::Orientation spawnOrientationOfTank(int tankID) const;

    explicit TankMap(QObject *parent = 0);
    ~TankMap();

    bool isLoaded() const { return _isLoaded; }

public slots:
    bool loadFile(const QString & filename);
    void lock() { _isLocked = true; }
    void unlock() { _isLocked = false; }

private:
    void findBases();

signals:
    //! The TankMap wants to display something
    void message(const QString & message);

private:
    quint32 _width;
    quint32 _height;
    quint32 _maxPlayers;
    quint32 _numTankPerPlayer;

    bool _isLoaded = false; // The last call to loadfile was successful
    bool _isLocked = false; // If true, loadFile won't load anything

    QVector<Cell> _cells;
    QVector<QVector<Position> > _spawnPositionsPerPlayer;
    QMap<quint32, Position> _spawnPositionPerTank;
    QMap<quint32, TankGame::Orientation> _spawnOrientationPerTank;
    QMap<char, Cell> _charToCell;
};

bool operator<(const TankMap::Position & p1, const TankMap::Position & p2);
QDebug & operator<<(QDebug dbg, const TankMap::Position & p);


#endif // TANKMAP_HPP
