#include "cell_game.hpp"

#include <QVector2D>

CellGame::CellGame()
{

}

CellGame::~CellGame()
{

}

void CellGame::onPlayerMove(Client *client, int turn, const QByteArray &data)
{
    int player_id = _playerClients.indexOf({client,true});
    Q_ASSERT(player_id != -1);

    Player * player = _players[player_id];
    Q_ASSERT(player->id == player_id);

    // todo: fill actions data structures (and check their validity before adding any action)
}

void CellGame::onVisuAck(Client *client, int turn, const QByteArray &data)
{
    // todo
}

void CellGame::onTurnEnd()
{
    /*
     * Actions are executed in this order:
     *   1. Cell divisions
     *   2. Virus creations
     *   3. Cell moves
     *   4. Player surrenders
     *
     * Each part is done in the order of received messages.
     *
     * Collisions are then computed to detect the cells that eat each other.
     *
     */
}

void CellGame::compute_cell_divisions()
{
    for (const DivideAction * action : _divide_actions)
    {
        // Internal assertions to avoid bugs
        Q_ASSERT(_player_cells.contains(action->cell_id));

        Cell * cell = _player_cells[action->cell_id];
        Q_ASSERT(action->new_cell_mass >= 2);
        Q_ASSERT(action->new_cell_mass <= cell->mass / 2);

        Q_ASSERT(_players.contains(cell->player_id));
        Q_ASSERT(_players[cell->player_id]->nb_cells < _parameters.max_cells_per_player);

        // Remove mass from the targetted cell (and update its radius, speed, etc.)
        cell->removeMass(action->new_cell_mass, _parameters);

        // Compute movement vector
        QVector2D move_vector(action->desired_new_cell_destination.x - cell->position.x,
                              action->desired_new_cell_destination.y - cell->position.y);

        // Let this vector be normalized (or set to point to the north if it is null)
        if (move_vector.isNull())
            move_vector.setY(-1);
        else
            move_vector.normalize();

        // Let the new cell position be computed
        double new_cell_radius = _parameters.compute_radius_from_mass(action->new_cell_mass);
        double distance = new_cell_radius + cell->radius;

        QVector2D new_cell_translation = move_vector * distance;

        // Let the new cell be created
        Cell * new_cell = new Cell;
        new_cell->id = next_cell_id();
        new_cell->player_id = cell->player_id;
        new_cell->position.x = cell->position.x + new_cell_translation.x();
        new_cell->position.y = cell->position.y + new_cell_translation.y();
        new_cell->updateMass(action->new_cell_mass, _parameters);
        new_cell->remaining_isolated_turns = 0;

        _player_cells[new_cell->id] = new_cell;

        // todo: handle map dimension limit

        delete action;
    }

    _divide_actions.clear();
}

void CellGame::compute_virus_creations()
{
    for (const CreateVirusAction * action : _create_virus_actions)
    {
        // Internal assertions to avoid bugs
        Q_ASSERT(_player_cells.contains(action->cell_id));

        Cell * cell = _player_cells[action->cell_id];
        double mass_loss = cell->mass * _parameters.virus_creation_mass_loss - _parameters.virus_mass;
        Q_ASSERT(cell->mass - mass_loss >= 2);

        // todo:check that there is no cell belonging to any other player in a given radius around the virus we want to create
        // todo:handle max number of viruses

        // Let some mass be removed from the cell
        cell->removeMass(mass_loss, _parameters);

        // Compute movement vector
        QVector2D move_vector(action->desired_virus_destination.x - cell->position.x,
                              action->desired_virus_destination.y - cell->position.y);

        // Let this vector be normalized (or set to point to the north if it is null)
        if (move_vector.isNull())
            move_vector.setY(-1);
        else
            move_vector.normalize();

        // Let the virus position be computed
        double virus_radius = _parameters.compute_radius_from_mass(_parameters.virus_mass);
        double distance = virus_radius + cell->radius;

        QVector2D virus_translation = move_vector * distance;

        // Let the new virus be created
        Virus * virus = new Virus;
        virus->id = next_cell_id();
        virus->position.x = cell->position.x + virus_translation.x();
        virus->position.y = cell->position.y + virus_translation.y();

        // todo: handle map dimension limit

        _viruses[virus->id] = virus;

        delete action;
    }

    _create_virus_actions.clear();
}

void CellGame::compute_cell_moves()
{
    for (const MoveAction * action : _move_actions)
    {
        // Internal assertions to avoid bugs
        Q_ASSERT(_player_cells.contains(action->cell_id));
        Cell * cell = _player_cells[action->cell_id];

        // Let the movement vector be computed
        QVector2D move_vector(action->desired_destination.x - cell->position.x,
                              action->desired_destination.y - cell->position.y);

        // If the cell is already at its destination, we don't do anything
        if (!move_vector.isNull())
        {
            // The cell will be moved along the move_vector direction.
            // If the cell can reach the desired destination, it stops at it.
            // Otherwise, the cell goes as far as possible on that direction.

            float desired_distance = move_vector.length();

            if (desired_distance <= cell->max_speed)
            {
                // If the cell can reach its destination
                cell->position = action->desired_destination;
            }
            else
            {
                // Otherwise, let us compute where the cell should stop
                move_vector /= desired_distance;
                cell->position.x = move_vector.x() * cell->max_speed;
                cell->position.y = move_vector.y() * cell->max_speed;
            }
        }

        delete action;
    }

    _move_actions.clear();
}

void CellGame::compute_player_surrenders()
{
    for (const SurrenderAction * action : _surrender_actions)
    {
        // Internal assertions
        Q_ASSERT(_players.contains(action->player_id));

        QVector<Cell *> cells_to_delete;
        // Let us mark all the player cells as neutral
        for (Cell * cell : _player_cells)
        {
            if (cell->player_id == action->player_id)
            {
                NeutralCell * neutral_cell = new NeutralCell;
                neutral_cell->id = cell->id;
                neutral_cell->is_initial = false;
                neutral_cell->mass = cell->mass;
                neutral_cell->position = cell->position;
                neutral_cell->radius = _parameters.compute_radius_from_mass(neutral_cell->mass);

                _alive_neutral_cells[neutral_cell->id] = neutral_cell;

                cells_to_delete.append(cell);
            }
        }

        for (Cell * cell : cells_to_delete)
        {
            delete cell;
        }

        // TODO: make player reappear

        delete action;
    }

    _surrender_actions.clear();
}

int CellGame::next_cell_id()
{
    int ret = _next_cell_id++;

    Q_ASSERT(!_player_cells.contains(ret));
    Q_ASSERT(!_initial_neutral_cells.contains(ret));
    Q_ASSERT(!_alive_neutral_cells.contains(ret));
    Q_ASSERT(!_viruses.contains(ret));

    // todo: handle case when 2^32-1 is reached + handle performance issues
    return ret;
}

void CellGame::Cell::eatCell(const CellGame::Cell *eaten_cell)
{
    //todo
}

void CellGame::Cell::updateMass(double new_mass, const CellGame::GameParameters &parameters)
{
    //todo: set new mass then compute radius, max speed etc.
}

void CellGame::Cell::addMass(double mass_increment, const CellGame::GameParameters &parameters)
{
    //todo
}

void CellGame::Cell::removeMass(double mass_decrement, const CellGame::GameParameters &parameters)
{
    //todo
}

bool CellGame::Cell::containsPosition(const CellGame::Position &position) const
{
    //todo
}

float CellGame::GameParameters::compute_radius_from_mass(float mass)
{
    return radius_factor * mass;
}
