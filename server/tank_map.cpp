#include "tank_map.hpp"

#include <QFile>
#include <QtAlgorithms>
#include <QDebug>

const TankMap::Cell & TankMap::at(const TankMap::Position &p) const
{
    return _cells[p.y * _width + p.x];
}

TankMap::Cell & TankMap::at(const TankMap::Position &p)
{
    return _cells[p.y * _width + p.x];
}

const TankMap::Cell &TankMap::at(quint32 x, quint32 y) const
{
    return _cells[y * _width + x];
}

TankMap::Cell &TankMap::at(quint32 x, quint32 y)
{
    return _cells[y * _width + x];
}

bool TankMap::isInMap(const TankMap::Position &p) const
{
    return p.x < _width && p.y < _height;
}

bool TankMap::isInMap(quint32 x, quint32 y) const
{
    return x < _width && y < _height;
}

TankMap::Position TankMap::spawnPositionOfTank(int tankID) const
{
    return _spawnPositionPerTank[tankID];
}

TankGame::Orientation TankMap::spawnOrientationOfTank(int tankID) const
{
    return _spawnOrientationPerTank[tankID];
}

TankMap::TankMap(QObject *parent) : QObject(parent)
{
    _charToCell[' '] = Cell::EMPTY;
    _charToCell['X'] = Cell::UNBREAKABLE_WALL;
    _charToCell['x'] = Cell::BREAKABLE_WALL;
    _charToCell['p'] = Cell::BONUS_POINTS;
    _charToCell['m'] = Cell::BONUS_MINES;
    _charToCell['d'] = Cell::BONUS_DAMAGE;
    _charToCell['b'] = Cell::BONUS_BIGSHOT;
    _charToCell['s'] = Cell::TANK_SPAWN;
}

TankMap::~TankMap()
{

}

bool TankMap::loadFile(const QString &filename)
{
    if (_isLocked)
    {
        return false;
    }

    QFile f(filename);

    if (!f.open(QIODevice::ReadOnly))
    {
        emit message(QString("Cannot open file '%1'").arg(filename));
        return false;
    }

    QString line = f.readLine().trimmed();
    quint32 width = line.size();
    _width = width;
    _height = 1;

    while (!f.atEnd())
    {
        line = f.readLine().trimmed();
        width = line.size();

        if (width != _width)
        {
            emit message(QString("Invalid file '%s': line %2 has width=%3 (expected %4)").arg(filename).arg(_height).arg(width).arg(_width));
            _isLoaded = false;
            return false;
        }

        _height++;
    }

    if (_width < 3 || _height < 3)
    {
        emit message(QString("Invalid file '%s': width and height must be at least 3 (got width=%1, height=%2)").arg(_width).arg(_height));
        _isLoaded = false;
        return false;
    }

    if (!f.reset())
    {
        emit message(QString("Impossible to reset file '%s'").arg(filename));
        _isLoaded = false;
        return false;
    }

    _cells.resize(_width * _height);

    for (quint32 y = 0; y < _height; ++y)
    {
        line = f.readLine().trimmed();

        for (quint32 x = 0; x < _width; ++x)
        {
            char c = line.at(x).toLatin1();
            if (!_charToCell.contains(c))
            {
                emit message(QString("Impossible to read file '%1' : invalid character '%2' at position (x=%3,y=%4)").arg(
                                    filename).arg(c).arg(x).arg(y));
                _isLoaded = false;
                return false;
            }
            _cells[y*_width+x] = Cell(c);
        }
    }

    // Let's find where the bases are
    findBases();

    // Let's check if the bases are valid (there must be at least 2 bases, all bases must have the same number of tanks)
    _maxPlayers = _spawnPositionsPerPlayer.size();

    if (_maxPlayers < 2)
    {
        emit message(QString("Invalid map '%1': there should be at least 2 bases in it"));
        _isLoaded = false;
        return false;
    }

    _numTankPerPlayer = _spawnPositionsPerPlayer.first().size();
    for (quint32 i = 0; i < _maxPlayers; ++i)
    {
        if ((quint32)_spawnPositionsPerPlayer[i].size() != _numTankPerPlayer)
        {
            emit message(QString("Invalid map '%1': all bases must have the same size"));
            _isLoaded = false;
            return false;
        }
    }

    // Let's compute the spawn positions and orientations of the tanks
    quint32 tankID = 0;
    for (quint32 player = 0; player < _maxPlayers; ++player)
    {
        for (quint32 i = 0; i < _numTankPerPlayer; ++i)
        {
            const Position & p = _spawnPositionsPerPlayer[player][i];
            _spawnPositionPerTank[tankID] = p;

            // Let's determine the orientation of the tank.
            // The tank should be adjacent (horizontally and vertically speaking) of 1 and only 1 UNBREAKABLE_WALL
            // Let's say the tank back is against this wall.

            int nbAdjacentUWalls = 0;
            struct Direction
            {
                TankGame::Orientation orientation;
                int x;
                int y;
            };

            static const Direction directions[] =
            {
                {TankGame::Orientation::NORTH, 0, -1},
                {TankGame::Orientation::WEST , 1,  0},
                {TankGame::Orientation::SOUTH, 0,  1},
                {TankGame::Orientation::EAST ,-1,  0}
            };

            for (int d = 0; d < 4; ++d)
            {
                const Direction & dir = directions[d];
                Position neighbor = {p.x + dir.x, p.y + dir.y};

                if (isInMap(neighbor) && at(neighbor) == Cell::UNBREAKABLE_WALL)
                {
                    _spawnOrientationPerTank[tankID] = dir.orientation;
                    ++nbAdjacentUWalls;
                }
            }

            if (nbAdjacentUWalls != 1)
            {
                emit message(QString("Invalid map '%1': base at (%2,%3) should have one and only one UNBREAKABLE_WALL neighbor (horizontally and vertically speaking), but got %4").arg(
                                filename).arg(p.x).arg(p.y).arg(nbAdjacentUWalls));
                _isLoaded = false;
                return false;
            }

            tankID++;
        }
    }

    emit message(QString("Map '%1' loaded successfully").arg(filename));
    _isLoaded = true;
    return true;
}

void TankMap::findBases()
{
    // A base is a set of adjecents cells of type TANK_SPAWN.
    // Adjacency is vertical, horizontal and diagonal

    QVector<Position> spawners;

    for(quint32 y = 0; y < _height; ++y)
    {
        for (quint32 x = 0; x < _width; ++x)
        {
            if (at(x,y) == Cell::TANK_SPAWN)
                spawners.push_back({x,y});
        }
    }

    _spawnPositionsPerPlayer.clear();
    _spawnPositionPerTank.clear();

    auto isAdjacent = [] (const Position & p1, const Position & p2)
    {
        int xdiff = abs((int)p1.x - (int)p2.x);
        int ydiff = abs((int)p1.y - (int)p2.y);

        if (xdiff == 1)
            return ydiff == 0 || ydiff == 1;
        else if (xdiff == 0)
            return ydiff == 0 || ydiff == 1;
        else
            return false;
    };

    while (!spawners.empty())
    {
        QVector<Position> base;

        base.push_back(spawners.back());
        spawners.pop_back();

        bool didSomething = false;

        do
        {
            didSomething = false;

            for (int i = 0; i < spawners.size(); ++i)
            {
                for (int j = 0; j < base.size(); ++j)
                {
                    if (isAdjacent(spawners[i], base[j]))
                    {
                        base.push_back(spawners[i]);
                        spawners.remove(i);
                        i--;
                        didSomething = true;
                        break;
                    }
                }
            }

        } while(didSomething);

        _spawnPositionsPerPlayer.push_back(base);
    }
}



bool operator<(const TankMap::Position &p1, const TankMap::Position &p2)
{
    if (p1.x == p2.x)
        return p1.y < p2.y;
    return p1.x < p2.x;
}


QDebug &operator<<(QDebug dbg, const TankMap::Position &p)
{
    dbg.nospace() << '(' << p.x << ',' << p.y << ')';
    return dbg.space();
}
