#include "cell_game.hpp"

#include <QVector2D>
#include <QQueue>

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

        PlayerCell * cell = _player_cells[action->cell_id];
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
        PlayerCell * new_cell = new PlayerCell;
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

        PlayerCell * cell = _player_cells[action->cell_id];
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
        PlayerCell * cell = _player_cells[action->cell_id];

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

        QVector<PlayerCell *> cells_to_delete;
        // Let us mark all the player cells as neutral
        for (PlayerCell * cell : _player_cells)
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

        for (PlayerCell * cell : cells_to_delete)
        {
            delete cell;
        }

        Player * player = _players[action->player_id];

        player->nb_cells -= cells_to_delete.size();
        Q_ASSERT(player->nb_cells == 0);

        make_player_reappear(player);

        delete action;
    }

    _surrender_actions.clear();
}

void CellGame::compute_cell_positions()
{
    // Is this necessary ? Might be computed when mass is modified.
    for (PlayerCell * cell : _player_cells)
    {
        QuadTreeNode * node = cell->responsible_node->find_responsible_node(cell->position);
        Q_ASSERT(node != nullptr);
        if (node != cell->responsible_node)
        {
            cell->responsible_node->player_cells.remove(cell->id);
            node->player_cells[cell->id] = cell;
            cell->responsible_node = node;
        }

        node = cell->responsible_node_bbox->find_responsible_node(cell->top_left, cell->bottom_right);
        Q_ASSERT(node != nullptr);
        if (node != cell->responsible_node_bbox)
        {
            cell->responsible_node_bbox->player_cells_bbox.remove(cell->id);
            node->player_cells_bbox[cell->id] = cell;
            cell->responsible_node_bbox = node;
        }
    }
}

void CellGame::compute_cell_collisions()
{
    QSet<PlayerCell *> cells_to_recompute;
    QSet<PlayerCell *> cells_to_delete;
    QSet<NeutralCell *> ncells_to_delete;
    QSet<Virus *> viruses_to_delete;
    bool did_something = false;

    // The first pass consists in computing, for each pcell "cell"
    //   - all possible collisions between "cell" and reachable player cells,
    //   - all possible collisions between "cell" and reachable neutral cells,
    //   - all possible collisions between "cell" and reachable viruses.
    for (PlayerCell * cell : _player_cells)
    {
        // If "cell" has already been eaten by some other pcell, let it be ignored
        if (cells_to_delete.contains(cell))
            continue;

        // The copy region starts here
        QuadTreeNode * node = cell->responsible_node_bbox;
        Q_ASSERT(node != nullptr);

        QQueue<QuadTreeNode *> node_queue;
        node_queue.enqueue(cell->responsible_node_bbox);

        while (!node_queue.isEmpty())
        {
            node = node_queue.dequeue();
            if (!node->is_leaf())
            {
                node_queue.enqueue(node->child_top_left);
                node_queue.enqueue(node->child_top_right);
                node_queue.enqueue(node->child_bottom_left);
                node_queue.enqueue(node->child_bottom_right);
            }

            // Let us check if "cell" collides with other player cells.
            // To do so, let us iterate over the oth_pcells whose position is inside the current node
            QMutableMapIterator<int, PlayerCell *> oth_pcell_it(node->player_cells);
            while (oth_pcell_it.hasNext())
            {
                oth_pcell_it.next();
                PlayerCell * oth_pcell = oth_pcell_it.value();
                float dist = cell->squared_distance_to(oth_pcell);

                // If oth_pcell is close enough to "cell" to be absorbed by it
                if (dist < cell->radius_squared)
                {
                    if (cell->player_id == oth_pcell->player_id)
                    {
                        if ((cell->id != oth_pcell->id) &&
                            (cell->remaining_isolated_turns == 0) &&
                            (oth_pcell->remaining_isolated_turns == 0))
                        {
                            // Two cells of the same player merge
                            Position g = compute_barycenter(cell->position, cell->mass, oth_pcell->position, oth_pcell->mass);
                            cell->position = g;
                            cell->addMass(oth_pcell->mass, _parameters);
                            cells_to_recompute.insert(cell);

                            cells_to_delete.insert(oth_pcell);
                            oth_pcell->responsible_node_bbox->player_cells_bbox.remove(oth_pcell->id);
                            oth_pcell_it.remove();

                            did_something  = true;
                        }
                    }
                    else
                    {
                        if (cell->mass > _parameters.minimum_mass_ratio_to_absorb * oth_pcell->mass)
                        {
                            // "cell" absorbs oth_pcell (not belonging to the same player)
                            Position g = compute_barycenter(cell->position, cell->mass, oth_pcell->position, oth_pcell->mass);
                            cell->position = g;
                            cell->addMass(oth_pcell->mass * _parameters.mass_absorption, _parameters);
                            cells_to_recompute.insert(cell);

                            cells_to_delete.insert(oth_pcell);
                            oth_pcell->responsible_node->player_cells_bbox.remove(oth_pcell->id);
                            oth_pcell_it.remove();

                            did_something = true;
                        }
                    }
                }
            } // end of iteration through the player cells of the current node

            // Let us now check if "cell" collides with neutral cells.
            // To do so, let us iterate over the ncells whose position is inside the current node
            QMutableMapIterator<int, NeutralCell *> ncell_it(node->alive_neutral_cells);
            while (ncell_it.hasNext())
            {
                ncell_it.next();
                NeutralCell * ncell = ncell_it.value();

                float dist = cell->squared_distance_to(ncell);

                // If ncell is close enough to "cell" to be absorbed by it
                if (dist < cell->radius_squared)
                {
                    // The ncell is absorbed by "cell"

                    Position g = compute_barycenter(cell->position, cell->mass, ncell->position, ncell->mass);
                    cell->position = g;
                    cell->addMass(ncell->mass * _parameters.mass_absorption, _parameters);
                    cells_to_recompute.insert(cell);

                    if (ncell->is_initial)
                    {
                        ncell->remaining_turns_before_apparition = _parameters.neutral_cells_repop_time;
                        ncell_it.remove();
                    }
                    else
                    {
                        ncells_to_delete.insert(ncell);
                        ncell_it.remove();
                    }
                }
            }

            // Let us now check if "cell" collides with viruses.
            // To do so, let us iterate over the viruses whose position is inside the current node
            QMutableMapIterator<int, Virus *> viruses_it(node->viruses);
            while (viruses_it.hasNext())
            {
                viruses_it.next();
                Virus * virus = viruses_it.value();

                float dist = cell->squared_distance_to(virus);

                if ((dist < cell->radius_squared) && (cell->mass > _parameters.virus_mass))
                {
                    // The poor "cell" just ate a virus :(
                    // First, let the virus mass be removed from the poor "cell"
                    cell->removeMass(_parameters.virus_mass, _parameters);
                    float total_mass = cell->mass;

                    // The central cell mass is divided by 2
                    cell->updateMass(total_mass / 2, _parameters);

                    // The central cell might be surrounded by up to virus_max_split satellite cells
                    // but the total number of cells cannot exceed max_cells_per_player
                    const int nb_satellites = std::min(_parameters.virus_max_split, _parameters.max_cells_per_player - _players[cell->player_id]->nb_cells);
                    Q_ASSERT(_players[cell->player_id]->nb_cells + nb_satellites <= _parameters.max_cells_per_player);
                    Q_ASSERT(nb_satellites >= 0);

                    if (nb_satellites > 0)
                    {
                        // Let the satellite mass be computed
                        const float mass_by_satellite = total_mass / 2;

                        // Let the distance from cell.center to satellite.center be computed
                        const float dist_to_satellite = cell->radius + _parameters.compute_radius_from_mass(mass_by_satellite);

                        // Let the angle (in radians) between each satellite be computed
                        const float angle_diff = (2 * M_PI) / nb_satellites;

                        for (int satellite_id = 0; satellite_id < nb_satellites; ++satellite_id)
                        {
                            PlayerCell * satellite = new PlayerCell;
                            satellite->id = next_cell_id();
                            satellite->player_id = cell->player_id;
                            satellite->position.x = cell->position.x + dist_to_satellite * std::cos(satellite_id * angle_diff);
                            satellite->position.y = cell->position.y + dist_to_satellite * std::sin(satellite_id * angle_diff);
                            satellite->updateMass(mass_by_satellite, _parameters);

                            _players[satellite->player_id]->nb_cells++;

                            // todo: add new cells in the data structures
                        }
                    }

                    viruses_to_delete.insert(virus);
                    viruses_it.remove();
                }
            }

            // The copy region ends here
        } // end of nodes traversal towards leaves
    }

    // The first pass has been done.
    // Let us recompute what should be done until stability is reached
    did_something = true;
    while (did_something)
    {
        did_something = false;

        for (PlayerCell * cell : cells_to_delete)
        {
            _player_cells.remove(cell->id);
            cell->responsible_node->player_cells.remove(cell->id);
            cell->responsible_node_bbox->player_cells_bbox.remove(cell->id);
            delete cell;
        }
        cells_to_delete.clear();

        // todo: delete neutral cells
        // todo: delete viruses

        QSet<PlayerCell *> cells = cells_to_recompute;
        cells_to_recompute.clear();

        for (PlayerCell * cell : cells)
        {
            // Let us check if the previous "cell" growth has caused it to be absorbed by a bigger pcell
            QuadTreeNode * growth_node = cell->responsible_node_bbox;
            bool current_cell_has_been_eaten = false;

            // We will traverse nodes towards the root except if "cell" has already been eaten
            while (growth_node != nullptr && !current_cell_has_been_eaten)
            {
                // Let us traverse all other pcells whose bbox is in the node (they might eat "cell")
                for (PlayerCell * oth_pcell : growth_node->player_cells_bbox)
                {
                    float dist = oth_pcell->squared_distance_to(cell);

                    // If the oth_pcell distance to "cell" may lead to "cell" being eaten
                    if (dist < oth_pcell->radius_squared)
                    {
                        if (cell->player_id == oth_pcell->player_id)
                        {
                            if ((cell->id != oth_pcell->id) &&
                                (cell->remaining_isolated_turns == 0) &&
                                (oth_pcell->remaining_isolated_turns == 0))
                            {
                                // Two cells of the same player merge
                                Position g = compute_barycenter(cell->position, cell->mass, oth_pcell->position, oth_pcell->mass);
                                oth_pcell->position = g;
                                oth_pcell->addMass(cell->mass, _parameters);
                                cells_to_recompute.insert(oth_pcell);

                                current_cell_has_been_eaten = true;
                                did_something  = true;
                            }
                        }
                        else
                        {
                            if (oth_pcell->mass > _parameters.minimum_mass_ratio_to_absorb * cell->mass)
                            {
                                // oth_pcell absorbs "cell" (not belonging to the same player)
                                Position g = compute_barycenter(cell->position, cell->mass, oth_pcell->position, oth_pcell->mass);
                                oth_pcell->position = g;
                                oth_pcell->addMass(cell->mass * _parameters.mass_absorption, _parameters);
                                cells_to_recompute.insert(oth_pcell);

                                did_something = true;
                                current_cell_has_been_eaten = true;
                            }
                        }
                    }

                    // If the current cell has been eaten, let the oth_pcells traversal be stopped
                    if (current_cell_has_been_eaten)
                        break;
                } // end of oth_pcells traversal inside current node

                growth_node = growth_node->parent;
            } // end of node traversal towards root

            if (current_cell_has_been_eaten)
            {
                // If "cell" has been eaten, we can remove it from the maps now.
                // (It could not have been done before because we were iterating over the maps)
                cell->responsible_node->player_cells.remove(cell->id);
                cell->responsible_node_bbox->player_cells_bbox.remove(cell->id);
                cells_to_delete.insert(cell);
            }
            else
            {
                // We now know that "cell" has not been eaten by a larger cell.
                // Let us now check if "cell" growth has caused it to eat a smaller cell.

                // The following code MUST be copy-pasted from the first pass to avoid errors!
                // DO NOT REMOVE "BEFORE PASTE" and "AFTER PASTE" comments.

                // BEFORE PASTE

                // AFTER PASTE
            }


            QQueue<QuadTreeNode *> node_queue;
            node_queue.enqueue(cell->responsible_node);

        } // end of traversal of nodes which have been modified the last round
    }
}

void CellGame::make_player_reappear(CellGame::Player *player)
{
    Q_ASSERT(player->nb_cells == 0);
    // generate nb_starting_cells_per_player positions randomly.
    // if the position is within another cell, regenerate it.

    // TODO
}

int CellGame::next_cell_id()
{
    int previous_cell_id = _next_cell_id;
    int ret = _next_cell_id++;

    // If we met reach the maximum integer
    if (previous_cell_id > ret)
        compress_cell_ids();

    return ret;
}

int CellGame::compress_cell_ids()
{
    // The first nb_neutral_cells IDs are taken for initial neutral cells.
    // The other ones can be compressed!

    enum CellType
    {
        NEUTRAL_CELL,
        PLAYER_CELL,
        VIRUS
    };

    struct IdTypeCell
    {
        int id;
        CellType type;
    };

    QVector<IdTypeCell> cells;
    // TODO
}

void CellGame::PlayerCell::eatCell(const CellGame::PlayerCell *eaten_cell)
{
    //todo
}

void CellGame::PlayerCell::updateMass(double new_mass, const CellGame::GameParameters &parameters)
{
    //todo: set new mass then compute radius, radius_squared, bbox, max speed etc.
    // beware: must ensure the mass is >= minimum_player_cell_mass
}

void CellGame::PlayerCell::addMass(double mass_increment, const CellGame::GameParameters &parameters)
{
    //todo -> call updateMass
}

void CellGame::PlayerCell::removeMass(double mass_decrement, const CellGame::GameParameters &parameters)
{
    //todo -> call updateMass

}

bool CellGame::PlayerCell::containsPosition(const CellGame::Position &position) const
{
    //todo
}

float CellGame::PlayerCell::squared_distance_to(const CellGame::PlayerCell *oth_cell) const
{
    float dx = this->position.x - oth_cell->position.x;
    float dy = this->position.y - oth_cell->position.y;
    return dx*dx + dy*dy;
}

float CellGame::PlayerCell::squared_distance_to(const CellGame::NeutralCell *ncell) const
{
    float dx = this->position.x - ncell->position.x;
    float dy = this->position.y - ncell->position.y;
    return dx*dx + dy*dy;
}

float CellGame::PlayerCell::squared_distance_to(const CellGame::Virus *virus) const
{
    float dx = this->position.x - virus->position.x;
    float dy = this->position.y - virus->position.y;
    return dx*dx + dy*dy;
}

float CellGame::GameParameters::compute_radius_from_mass(float mass)
{
    return radius_factor * mass;
}

CellGame::QuadTreeNode::QuadTreeNode(unsigned int depth, Position top_left,
                                     Position bottom_right, CellGame::QuadTreeNode *parent)
{
    Q_ASSERT(depth > 0);
    this->parent = parent;
    this->depth = depth;
    top_left_position = top_left;
    bottom_right_position = bottom_right;
    Q_ASSERT(top_left_position.x < bottom_right_position.x);
    Q_ASSERT(top_left_position.y < bottom_right_position.y);

    mid_position.x = (top_left.x + bottom_right.x) / 2;
    mid_position.y = (top_left.y + bottom_right.y) / 2;

    if (depth > 1)
    {
        child_top_left = new QuadTreeNode(depth - 1, top_left_position, mid_position, this);
        child_top_right = new QuadTreeNode(depth - 1, Position(mid_position.x, top_left_position.y), Position(bottom_right_position.x, mid_position.y), this);
        child_bottom_left = new QuadTreeNode(depth - 1, Position(top_left_position.x, mid_position.y), Position(mid_position.x, bottom_right_position.y), this);
        child_bottom_right = new QuadTreeNode(depth - 1, mid_position, bottom_right_position, this);
    }
}

bool CellGame::QuadTreeNode::contains_position(const CellGame::Position &position) const
{
    return (position.x >= top_left_position.x) && (position.y >= top_left_position.y) &&
           (position.x < bottom_right_position.x) && (position.y < bottom_right_position.y);
}

bool CellGame::QuadTreeNode::contains_rectangle(const CellGame::Position &top_left,
                                                const CellGame::Position &bottom_right) const
{
    return (top_left.x >= top_left_position.x) && (top_left.y >= top_left_position.y) &&
            (bottom_right.x < bottom_right_position.x) && (bottom_right.y < bottom_right_position.y);
}

bool CellGame::QuadTreeNode::is_leaf() const
{
    return depth == 0;
}

bool CellGame::QuadTreeNode::is_root() const
{
    return parent == nullptr;
}

CellGame::QuadTreeNode *CellGame::QuadTreeNode::find_responsible_node(const CellGame::Position &position)
{
    return find_responsible_node_r(position);
}

CellGame::QuadTreeNode *CellGame::QuadTreeNode::find_responsible_node(const CellGame::Position &top_left, const CellGame::Position &bottom_right)
{
    Q_ASSERT(top_left.x < bottom_right.x);
    Q_ASSERT(top_left.y < bottom_right.y);

    return find_responsible_node_r(top_left, bottom_right);
}

CellGame::QuadTreeNode *CellGame::QuadTreeNode::find_responsible_node_r(const CellGame::Position &position)
{
    if (contains_position(position))
    {
        // The position is in the current node (or in one of its children)
        // If the current node is a leaf, let it be returned
        if (is_leaf())
            return this;
        else
        {
            // The current node is not a leaf. Let's call this method recursively on the right child
            if (position.x < mid_position.x)
            {
                if (position.y < mid_position.y)
                    return child_top_left->find_responsible_node_r(position);
                else
                    return child_bottom_left->find_responsible_node_r(position);
            }
            else
            {
                if (position.y < mid_position.y)
                    return child_top_right->find_responsible_node_r(position);
                else
                    return child_bottom_right->find_responsible_node_r(position);
            }
        }
    }
    else
    {
        // The position is not in the current node...
        // If the current node is the root, nullptr is returned.
        // Otherwise, let's call this method recursively on our parent
        if (is_root())
            return nullptr;
        else
            return parent->find_responsible_node_r(position);
    }
}

CellGame::QuadTreeNode *CellGame::QuadTreeNode::find_responsible_node_r(const CellGame::Position &top_left,
                                                                      const CellGame::Position &bottom_right)
{
    if (contains_rectangle(top_left, bottom_right))
    {
        // The rectangle is in the current node (or in one of its children)
        // If the current node is a leaf, let it be returned
        if (is_leaf())
            return this;
        else
        {
            // The current node is not a leaf. Let's check if the rectangle fits in one of our children
            if (bottom_right.x < mid_position.x)
            {
                if (bottom_right.y < mid_position.y)
                    return child_top_left->find_responsible_node_r(top_left, bottom_right);
                else if (top_left.y >= mid_position.y)
                    return child_bottom_left->find_responsible_node_r(top_left, bottom_right);
            }
            else if (top_left.x >= mid_position.x)
            {
                if (bottom_right.y < mid_position.y)
                    return child_top_right->find_responsible_node_r(top_left, bottom_right);
                else if (top_left.y >= mid_position.y)
                    return child_bottom_right->find_responsible_node_r(top_left, bottom_right);
            }

            return this;
        }
    }
    else
    {
        // The rectangle is not in the current node...
        // If the current node is the root, nullptr is returned.
        // Otherwise, let's call this method recursively on our parent
        if (is_root())
            return nullptr;
        else
            return parent->find_responsible_node_r(top_left, bottom_right);
    }
}

CellGame::Position::Position(int x, int y) :
    x(x), y(y)
{
}

CellGame::Position CellGame::compute_barycenter(const Position & a, float cA, const Position & b, float cB)
{
    Q_ASSERT(cA + cB != 0);

    // vect(A->G) = (b/(a+b)) * vect(A->B)
    float c = cB / (cA + cB);

    float dx = b.x - a.x;
    float dy = b.y - a.y;

    return Position(a.x + c*dx,
                    a.y + c*dy);
}
