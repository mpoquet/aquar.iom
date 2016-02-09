#include "cell_game.hpp"

#include <QVector2D>
#include <QQueue>

#include <cmath>

using namespace std;

CellGame::CellGame()
{

}

CellGame::~CellGame()
{

}

void CellGame::onPlayerMove(Client *client, int turn, QByteArray &data)
{
    (void) turn;
    int player_id = _playerClients.indexOf({client,true});
    Q_ASSERT(player_id != -1);

    Player * player = _players[player_id];
    Q_ASSERT(player->id == (quint32)player_id);

    // Message parsing
    char * reader = data.data();
    const char * data_end = reader + data.size();

    // Reading move actions
    if (reader + sizeof(quint32) > data_end)
    {
        emit wantToKick(client, "Invalid TURN_ACK GDC: cannot read nb_move_actions");
        return;
    }
    quint32 nb_move_actions = *((quint32*)reader);
    reader += sizeof(quint32);

    if (reader + nb_move_actions * (sizeof(quint32) + sizeof(float)*2) > data_end)
    {
        emit wantToKick(client, "Invalid TURN_ACK GDC: cannot read move actions");
        return;
    }

    for (quint32 i = 0; i < nb_move_actions; ++i)
    {
        MoveAction * move_action = new MoveAction;

        move_action->cell_id = *((quint32*)reader);
        reader += sizeof(quint32);
        move_action->desired_destination.x = *((float*)reader);
        reader += sizeof(float);
        move_action->desired_destination.y = *((float*)reader);
        reader += sizeof(float);

        // If the pcell does not exist, the action is ignored
        if (_player_cells.contains(move_action->cell_id))
        {
            PlayerCell * pcell = _player_cells[move_action->cell_id];
            Q_ASSERT(pcell->id == move_action->cell_id);
            // If the pcell does not belong to the client, the action is ignored
            if (pcell->player_id != (quint32)player_id)
                delete move_action;
            else
            {
                // If the coordinates are not finite, the action is ignored
                if (isfinite(move_action->desired_destination.x) &&
                    isfinite(move_action->desired_destination.y))
                    _move_actions.append(move_action);
                else
                    delete move_action;
            }
        }
        else
            delete move_action;
    }

    // Reading divide actions
    if (reader + sizeof(quint32) > data_end)
    {
        emit wantToKick(client, "Invalid TURN_ACK GDC: cannot read nb_divide_actions");
        return;
    }

    quint32 nb_divide_actions = *((quint32*)reader);
    reader += sizeof(quint32);

    if (reader + nb_divide_actions * (sizeof(quint32) + sizeof(float)*2 + sizeof(float)) > data_end)
    {
        emit wantToKick(client, "Invalid TURN_ACK GDC: cannot read divide actions");
        return;
    }

    for (quint32 i = 0; i < nb_divide_actions; ++i)
    {
        DivideAction * divide_action = new DivideAction;

        divide_action->cell_id = *((quint32*)reader);
        reader += sizeof(quint32);
        divide_action->desired_new_cell_destination.x = *((float*)reader);
        reader += sizeof(float);
        divide_action->desired_new_cell_destination.y = *((float*)reader);
        reader += sizeof(float);
        divide_action->new_cell_mass = *((float*)reader);
        reader += sizeof(float);

        // If the pcell does not exist, the action is ignored
        if (_player_cells.contains(divide_action->cell_id))
        {
            PlayerCell * pcell = _player_cells[divide_action->cell_id];
            Q_ASSERT(pcell->id == divide_action->cell_id);
            // If the pcell does not belong to the client, the action is ignored
            if (pcell->player_id != (quint32)player_id)
                delete divide_action;
            else
            {
                // If the coordinates are not finite, the action is ignored
                if (isfinite(divide_action->desired_new_cell_destination.x) &&
                    isfinite(divide_action->desired_new_cell_destination.y))
                    _divide_actions.append(divide_action);
                else
                    delete divide_action;
            }
        }
        else
            delete divide_action;
    }

    // Reading create virus actions
    if (reader + sizeof(quint32) > data_end)
    {
        emit wantToKick(client, "Invalid TURN_ACK GDC: cannot read nb_cvirus_actions");
        return;
    }

    quint32 nb_cvirus_actions = *((quint32*)reader);
    reader += sizeof(quint32);

    if (reader + nb_cvirus_actions * (sizeof(quint32) + sizeof(float)*2) > data_end)
    {
        emit wantToKick(client, "Invalid TURN_ACK GDC: cannot read create virus actions");
        return;
    }

    for (quint32 i = 0; i < nb_cvirus_actions; ++i)
    {
        CreateVirusAction * cvirus_action = new CreateVirusAction;

        cvirus_action->cell_id = *((quint32*)reader);
        reader += sizeof(quint32);
        cvirus_action->desired_virus_destination.x = *((float*)reader);
        reader += sizeof(float);
        cvirus_action->desired_virus_destination.y = *((float*)reader);
        reader += sizeof(float);

        // If the pcell does not exist, the action is ignored
        if (_player_cells.contains(cvirus_action->cell_id))
        {
            PlayerCell * pcell = _player_cells[cvirus_action->cell_id];
            Q_ASSERT(pcell->id == cvirus_action->cell_id);
            // If the pcell does not belong to the client, the action is ignored
            if (pcell->player_id != (quint32)player_id)
                delete cvirus_action;
            else
            {
                // If the coordinates are not finite, the action is ignored
                if (isfinite(cvirus_action->desired_virus_destination.x) &&
                    isfinite(cvirus_action->desired_virus_destination.y))
                    _create_virus_actions.append(cvirus_action);
                else
                    delete cvirus_action;
            }
        }
        else
            delete cvirus_action;
    }

    if (reader + sizeof(quint8) > data_end)
    {
        emit wantToKick(client, "Invalid TURN_ACK GDC: cannot read surrender boolean");
        return;
    }

    quint8 surrender = *((quint8*)reader);
    if (surrender == 1)
    {
        SurrenderAction * surrender_action = new SurrenderAction;
        surrender_action->player_id = (quint32) player_id;
        _surrender_actions.append(surrender_action);
    }
}

void CellGame::onVisuAck(Client *client, int turn, QByteArray &data)
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
     * Then, the following things are computed:
     *   5. Mass loss of every pcell
     *   6. Update of remaining isolated turns
     *   7. Update of dead neutral cells
     *   8. Collisions between cells
     *
     */

    compute_cell_divisions();
    compute_virus_creations();
    compute_cell_moves();
    compute_player_surrenders();

    compute_mass_loss();
    update_pcells_remaining_isolated_turns();
    update_dead_neutral_cells();
    compute_cell_collisions();

    _divide_actions.clear();
    _create_virus_actions.clear();
    _move_actions.clear();
    _surrender_actions.clear();

    ++_current_turn;

    //todo : send the new map to everyone
}

void CellGame::compute_cell_divisions()
{
    for (const DivideAction * action : _divide_actions)
    {
        // Internal assertions to avoid bugs
        Q_ASSERT(_player_cells.contains(action->cell_id));
        Q_ASSERT(_players.contains(_player_cells[action->cell_id]->player_id));

        PlayerCell * cell = _player_cells[action->cell_id];
        Player * player = _players[cell->player_id];

        // If the new cell mass is too low, the action is ignored
        if (action->new_cell_mass < _parameters.minimum_pcell_mass)
        {
            delete action;
            continue;
        }

        // If the cell is not big enough to be split, the action is ignored
        if (cell->mass < _parameters.minimum_pcell_mass * 2)
        {
            delete action;
            continue;
        }

        // If the new cell mass is greater than the half of the cell mass, the action is ignored
        if (action->new_cell_mass > cell->mass / 2)
        {
            delete action;
            continue;
        }

        // If the player has already reached its maximum number of cells, the action is ignored
        if (player->nb_cells >= _parameters.max_cells_per_player)
        {
            delete action;
            continue;
        }

        // The action seems to be valid!
        // Remove mass from the targetted cell (and update its radius, speed, etc.)
        cell->removeMass(action->new_cell_mass, _parameters);
        cell->updateQuadtreeNodes();

        // Compute movement vector
        QVector2D move_vector(action->desired_new_cell_destination.x - cell->position.x,
                              action->desired_new_cell_destination.y - cell->position.y);

        // Let this vector be normalized (or set to point to the north if it is null)
        if (move_vector.isNull())
            move_vector.setY(-1);
        else
            move_vector.normalize();

        // Let the new cell position be computed
        float new_cell_radius = _parameters.compute_radius_from_mass(action->new_cell_mass);
        float distance = new_cell_radius + cell->radius;

        QVector2D new_cell_translation = move_vector * distance;

        // Let the new cell be created
        PlayerCell * new_cell = new PlayerCell;
        new_cell->id = next_cell_id();
        new_cell->player_id = cell->player_id;
        new_cell->position.x = std::max(0.f, std::min(cell->position.x + new_cell_translation.x(), _parameters.map_width));
        new_cell->position.y = std::max(0.f, std::min(cell->position.y + new_cell_translation.y(), _parameters.map_height));
        new_cell->updateMass(action->new_cell_mass, _parameters);
        new_cell->responsible_node = _tree_root;
        new_cell->responsible_node_bbox = _tree_root;
        new_cell->updateQuadtreeNodes();
        new_cell->remaining_isolated_turns = 0;

        _player_cells[new_cell->id] = new_cell;

        player->nb_cells++;

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

        // If viruses cannot exist in the current parameters, the action is ignored
        if (_parameters.max_viruses <= 0)
        {
            delete action;
            continue;
        }

        PlayerCell * cell = _player_cells[action->cell_id];
        float mass_loss = cell->mass * _parameters.virus_creation_mass_loss - _parameters.virus_mass;

        // If the cell cannot remain alive by creating the virus, the action is ignored
        if (cell->mass - mass_loss <= 0)
        {
            delete action;
            continue;
        }

        // Let the virus attributes be computed
        float virus_radius = _parameters.compute_radius_from_mass(_parameters.virus_mass);
        float cell_radius_after_loss = _parameters.compute_radius_from_mass(cell->mass - mass_loss);

        // Movement vector
        QVector2D move_vector(action->desired_virus_destination.x - cell->position.x,
                              action->desired_virus_destination.y - cell->position.y);

        // Let this vector be normalized (or set it to point to the north if it is null)
        if (move_vector.isNull())
            move_vector.setY(-1);
        else
            move_vector.normalize();

        // Let the virus position be computed
        float distance = virus_radius + cell_radius_after_loss;

        Position virus_position(std::max(0.f, std::min(cell->position.x + move_vector.x() * distance, _parameters.map_width)),
                                std::max(0.f, std::min(cell->position.y + move_vector.y() * distance, _parameters.map_height)));

        // If there is an opponent near (in a 3*virus_radius radius) the new virus, the action is ignored
        if (is_there_opponent_pcell_in_neighbourhood(virus_position, 3*virus_radius, cell->player_id))
        {
            delete action;
            continue;
        }

        if (_viruses.size() >= (int)_parameters.max_viruses)
        {
            Q_ASSERT(_viruses.size() > 0);

            quint32 id_min = _viruses.first()->id;

            for (Virus * virus : _viruses)
            {
                if (virus->turn_of_birth < _viruses[id_min]->turn_of_birth)
                    id_min = virus->id;
                else if ((virus->turn_of_birth == _viruses[id_min]->turn_of_birth) &&
                         (virus->id < id_min))
                    id_min = virus->id;
            }

            Virus * virus_to_delete = _viruses[id_min];
            _viruses.remove(virus_to_delete->id);
            virus_to_delete->responsible_node->viruses.remove(virus_to_delete->id);
            delete virus_to_delete;
        }

        // Let some mass be removed from the cell
        cell->removeMass(mass_loss, _parameters);
        cell->updateQuadtreeNodes();

        // Let the new virus be created
        Virus * virus = new Virus;
        virus->id = next_cell_id();
        virus->position = virus_position;
        virus->turn_of_birth = _current_turn;
        virus->responsible_node = _tree_root->find_responsible_node(virus->position);

        virus->responsible_node->viruses[virus->id] = virus;
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
                cell->position.x = std::max(0.0f, std::min(action->desired_destination.x, _parameters.map_width));
                cell->position.y = std::max(0.0f, std::min(action->desired_destination.y, _parameters.map_height));
            }
            else
            {
                // Otherwise, let us compute where the cell should stop
                move_vector /= desired_distance;
                cell->position.x = std::max(0.0f, std::min(move_vector.x() * cell->max_speed, _parameters.map_width));
                cell->position.y = std::max(0.0f, std::min(move_vector.y() * cell->max_speed, _parameters.map_height));
            }

            cell->updateBBox(_parameters);
            cell->updateQuadtreeNodes();
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
                neutral_cell->responsible_node = cell->responsible_node;

                _alive_neutral_cells[neutral_cell->id] = neutral_cell;

                cells_to_delete.append(cell);
            }
        }

        for (PlayerCell * cell : cells_to_delete)
        {
            cell->responsible_node->player_cells.remove(cell->id);
            cell->responsible_node_bbox->player_cells_bbox.remove(cell->id);
            _player_cells.remove(cell->id);
            delete cell;
        }

        Player * player = _players[action->player_id];

        player->nb_cells -= cells_to_delete.size();
        Q_ASSERT(player->nb_cells == 0);

        make_player_pop(player);

        delete action;
    }

    _surrender_actions.clear();
}

void CellGame::compute_mass_loss()
{
    for (PlayerCell * cell : _player_cells)
    {
        cell->removeMass(cell->mass * _parameters.mass_loss_per_frame, _parameters);
        cell->updateQuadtreeNodes();
    }
}

void CellGame::update_pcells_remaining_isolated_turns()
{
    for (PlayerCell * cell : _player_cells)
    {
        if (cell->remaining_isolated_turns > 0)
            cell->remaining_isolated_turns--;
    }
}

void CellGame::update_dead_neutral_cells()
{
    QMutableListIterator<NeutralCell *> ncell_it(_dead_initial_neutral_cells);

    while (ncell_it.hasNext())
    {
        ncell_it.next();
        NeutralCell * ncell = ncell_it.value();

        ncell->remaining_turns_before_apparition--;

        if (ncell->remaining_turns_before_apparition == 0)
        {
            _alive_neutral_cells[ncell->id] = ncell;
            ncell->responsible_node->alive_neutral_cells[ncell->id] = ncell;
            ncell_it.remove();
        }
    }
}

void CellGame::compute_cell_collisions()
{
    QSet<PlayerCell *> pcells_to_recompute;
    QSet<PlayerCell *> pcells_to_delete;
    QSet<PlayerCell *> pcells_to_create;
    QSet<NeutralCell *> ncells_to_delete;
    QSet<Virus *> viruses_to_delete;
    bool did_something;

    compute_first_collision_pass(pcells_to_recompute, pcells_to_delete, pcells_to_create,
                                 ncells_to_delete, viruses_to_delete, did_something);

    // The first pass has been done.
    // Let us recompute what should be done until stability is reached
    while (did_something)
    {
        did_something = false;

        for (PlayerCell * cell : pcells_to_create)
        {
            _player_cells[cell->id] = cell;
        }
        pcells_to_create.clear();

        for (PlayerCell * cell : pcells_to_delete)
        {
            _player_cells.remove(cell->id);
            delete cell;
        }
        pcells_to_delete.clear();

        for (NeutralCell * ncell : ncells_to_delete)
        {
            _alive_neutral_cells.remove(ncell->id);
            delete ncell;
        }
        ncells_to_delete.clear();

        for (Virus * virus : viruses_to_delete)
        {
            _viruses.remove(virus->id);
            delete virus;
        }
        viruses_to_delete.clear();

        QSet<PlayerCell *> pcells = pcells_to_recompute;
        pcells_to_recompute.clear();

        for (PlayerCell * cell : pcells)
        {
            cell->updateBBox(_parameters);
            cell->updateQuadtreeNodes();
        }

        for (PlayerCell * cell : pcells)
        {
            // If "cell" has already been eaten by some other pcell, let it be ignored
            if (pcells_to_delete.contains(cell))
                continue;

            // Let us check if the previous "cell" growth has caused it to be absorbed by a bigger pcell or a virus
            QuadTreeNode * growth_node = cell->responsible_node_bbox;
            bool current_cell_has_been_eaten = false;

            // We will traverse nodes towards the root except if "cell" has already been eaten
            while (growth_node != nullptr && !current_cell_has_been_eaten)
            {
                current_cell_has_been_eaten = compute_pcell_outer_collisions_inside_node(cell, growth_node,
                                                                                         pcells_to_recompute,
                                                                                         did_something);
                growth_node = growth_node->parent;
            } // end of node traversal towards root

            if (current_cell_has_been_eaten)
            {
                // If "cell" has been eaten, we can remove it from the data structures
                cell->responsible_node->player_cells.remove(cell->id);
                cell->responsible_node_bbox->player_cells_bbox.remove(cell->id);
                pcells_to_delete.insert(cell);
            }
            else
            {
                // We now know that "cell" has not been eaten by a larger cell.
                // Let us now check if "cell" growth has caused it to eat a smaller cell.
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

                    compute_pcells_collisions_inside_node(cell, node, pcells_to_recompute, pcells_to_delete, did_something);
                    compute_ncells_collisions_inside_node(cell, node, pcells_to_recompute, ncells_to_delete, did_something);
                    compute_viruses_collisions_inside_node(cell, node, pcells_to_recompute, pcells_to_create, viruses_to_delete, did_something);
                }
            }
        } // end of traversal of nodes which have been modified the last round
    }
}

void CellGame::compute_first_collision_pass(QSet<CellGame::PlayerCell *> & pcells_to_recompute,
                                            QSet<CellGame::PlayerCell *> & pcells_to_delete,
                                            QSet<CellGame::PlayerCell *> & pcells_to_create,
                                            QSet<CellGame::NeutralCell *> & ncells_to_delete,
                                            QSet<CellGame::Virus *> & viruses_to_delete,
                                            bool &did_something)
{
    pcells_to_recompute.clear();
    pcells_to_delete.clear();
    pcells_to_create.clear();
    ncells_to_delete.clear();
    viruses_to_delete.clear();
    did_something = false;

    // The first pass consists in computing, for each pcell "cell"
    //   - all possible collisions between "cell" and reachable player cells,
    //   - all possible collisions between "cell" and reachable neutral cells,
    //   - all possible collisions between "cell" and reachable viruses.
    for (PlayerCell * cell : _player_cells)
    {
        // If "cell" has already been eaten by some other pcell, let it be ignored
        if (pcells_to_delete.contains(cell))
            continue;

        // Let the nodes be traversed towards leaves to test collisions with objects
        // that can touch "cell"'s bounding box
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

            compute_pcells_collisions_inside_node(cell, node, pcells_to_recompute, pcells_to_delete, did_something);
            compute_ncells_collisions_inside_node(cell, node, pcells_to_recompute, ncells_to_delete, did_something);
            compute_viruses_collisions_inside_node(cell, node, pcells_to_recompute, pcells_to_create, viruses_to_delete, did_something);
        }
    }
}

void CellGame::compute_pcells_collisions_inside_node(CellGame::PlayerCell * cell,
                                                     CellGame::QuadTreeNode * node,
                                                     QSet<CellGame::PlayerCell *> & pcells_to_recompute,
                                                     QSet<CellGame::PlayerCell *> & pcells_to_delete,
                                                     bool & did_something)
{
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
                    pcells_to_recompute.insert(cell);

                    pcells_to_delete.insert(oth_pcell);
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
                    pcells_to_recompute.insert(cell);

                    pcells_to_delete.insert(oth_pcell);
                    oth_pcell->responsible_node->player_cells_bbox.remove(oth_pcell->id);
                    oth_pcell_it.remove();

                    did_something = true;
                }
            }
        }
    } // end of iteration through the player cells of the current node
}

void CellGame::compute_ncells_collisions_inside_node(CellGame::PlayerCell * cell,
                                                     CellGame::QuadTreeNode * node,
                                                     QSet<CellGame::PlayerCell *> & pcells_to_recompute,
                                                     QSet<CellGame::NeutralCell *> & ncells_to_delete,
                                                     bool & did_something)
{
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
            pcells_to_recompute.insert(cell);
            did_something = true;

            if (ncell->is_initial)
            {
                ncell->remaining_turns_before_apparition = _parameters.neutral_cells_repop_time;
                Q_ASSERT(ncell->remaining_turns_before_apparition > 0);
                _dead_initial_neutral_cells.append(ncell);
                _alive_neutral_cells.remove(ncell->id);
                ncell_it.remove();
            }
            else
            {
                ncells_to_delete.insert(ncell);
                _alive_neutral_cells.remove(ncell->id);
                ncell_it.remove();
            }
        }
    }
}

void CellGame::compute_viruses_collisions_inside_node(CellGame::PlayerCell * cell,
                                                      CellGame::QuadTreeNode * node,
                                                      QSet<CellGame::PlayerCell *> & pcells_to_recompute,
                                                      QSet<CellGame::PlayerCell *> & pcells_to_create,
                                                      QSet<CellGame::Virus *> & viruses_to_delete,
                                                      bool & did_something)
{
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
            did_something = true;

            // The central cell mass is divided by 2
            Q_ASSERT(total_mass / 2 >= _parameters.minimum_pcell_mass);
            cell->updateMass(total_mass / 2, _parameters);
            pcells_to_recompute.insert(cell);

            // The central cell might be surrounded by up to virus_max_split satellite cells
            // but the total number of cells cannot exceed max_cells_per_player
            const int nb_satellites = std::min(_parameters.virus_max_split, _parameters.max_cells_per_player - _players[cell->player_id]->nb_cells);
            Q_ASSERT(_players[cell->player_id]->nb_cells + nb_satellites <= _parameters.max_cells_per_player);
            Q_ASSERT(nb_satellites >= 0);

            if (nb_satellites > 0)
            {
                // Let the satellite mass be computed
                const float mass_by_satellite = total_mass / 2;
                Q_ASSERT(mass_by_satellite >= _parameters.minimum_pcell_mass);

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

                    satellite->responsible_node = cell->responsible_node->find_responsible_node(satellite->position);
                    satellite->responsible_node_bbox = cell->responsible_node_bbox->find_responsible_node(satellite->top_left, satellite->bottom_right);
                    pcells_to_create.insert(satellite);
                    pcells_to_recompute.insert(satellite);
                }
            }

            viruses_to_delete.insert(virus);
            viruses_it.remove();
        }
    }
}

bool CellGame::compute_pcell_outer_collisions_inside_node(CellGame::PlayerCell * cell,
                                                          CellGame::QuadTreeNode * node,
                                                          QSet<CellGame::PlayerCell *> & pcells_to_recompute,
                                                          bool & did_something)
{
    // Let us traverse all other pcells whose bbox is in the node (they might eat "cell")
    for (PlayerCell * oth_pcell : node->player_cells_bbox)
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
                    pcells_to_recompute.insert(oth_pcell);

                    did_something = true;
                    return true;
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
                    pcells_to_recompute.insert(oth_pcell);

                    did_something = true;
                    return true;
                }
            }
        }
    }

    return false;
}

bool CellGame::is_there_opponent_pcell_in_neighbourhood(const CellGame::Position &position, float radius, quint32 player_id)
{
    Position top_left(position.x - radius, position.y - radius);
    Position bottom_right(position.x + radius, position.y + radius);
    float radius_squared = radius * radius;

    // Let the deepest node in which the bbox fits be found
    QuadTreeNode * node = _tree_root->find_responsible_node(top_left, bottom_right);
    Q_ASSERT(node != nullptr);

    // Let the nodes be traversed towards the root
    while (node != nullptr)
    {
        for (PlayerCell * oth_pcell : node->player_cells_bbox)
        {
            float dist = oth_pcell->squared_distance_to(position);
            if (((dist - oth_pcell->radius_squared) < radius_squared) && (oth_pcell->player_id != player_id))
                return true;
        }

        node = node->parent;
    }

    return false;
}

void CellGame::make_player_repop(CellGame::Player *player)
{

}

void CellGame::make_player_pop(CellGame::Player *player)
{
    Q_ASSERT(player->nb_cells == 0);
    // generate nb_starting_cells_per_player positions randomly.
    // if the position is within another cell, regenerate it.

    // TODO
}

quint32 CellGame::next_cell_id()
{
    quint32 previous_cell_id = _next_cell_id;
    quint32 ret = _next_cell_id++;

    // If we met reach the maximum integer
    if (previous_cell_id > ret)
    {
        compress_cell_ids();
        // todo: find what should be returned
    }

    return ret;
}

int CellGame::compress_cell_ids()
{
    // The first nb_neutral_cells IDs are taken for initial neutral cells.
    // The other ones can be compressed!
    Q_ASSERT(false);

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

void CellGame::PlayerCell::updateMass(float new_mass, const CellGame::GameParameters &parameters)
{
    mass = std::max(new_mass, parameters.minimum_pcell_mass);

    radius = parameters.compute_radius_from_mass(mass);
    radius_squared = radius * radius;
    max_speed = parameters.compute_max_speed_from_mass(mass);

    updateBBox(parameters);
}

void CellGame::PlayerCell::addMass(float mass_increment, const CellGame::GameParameters &parameters)
{
    updateMass(mass + mass_increment, parameters);
}

void CellGame::PlayerCell::removeMass(float mass_decrement, const CellGame::GameParameters &parameters)
{
    updateMass(mass - mass_decrement, parameters);
}

void CellGame::PlayerCell::updateBBox(const CellGame::GameParameters & parameters)
{
    top_left.x = std::max(0.0f, position.x - radius);
    top_left.y = std::max(0.0f, position.y - radius);

    bottom_right.y = std::min(position.x + radius, parameters.map_width);
    bottom_right.y = std::min(position.y + radius, parameters.map_height);
}

void CellGame::PlayerCell::updateQuadtreeNodes()
{
    responsible_node->player_cells.remove(id);
    responsible_node_bbox->player_cells_bbox.remove(id);

    responsible_node = responsible_node->find_responsible_node(position);
    responsible_node_bbox = responsible_node_bbox->find_responsible_node(top_left, bottom_right);
}

float CellGame::PlayerCell::squared_distance_to(const CellGame::PlayerCell *oth_pcell) const
{
    return squared_distance_to(oth_pcell->position);
}

float CellGame::PlayerCell::squared_distance_to(const CellGame::NeutralCell *ncell) const
{
    return squared_distance_to(ncell->position);
}

float CellGame::PlayerCell::squared_distance_to(const CellGame::Virus *virus) const
{
    return squared_distance_to(virus->position);
}

float CellGame::PlayerCell::squared_distance_to(const CellGame::Position &pos) const
{
    float dx = this->position.x - pos.x;
    float dy = this->position.y - pos.y;
    return dx*dx + dy*dy;
}

float CellGame::GameParameters::compute_radius_from_mass(float mass) const
{
    return radius_factor * mass;
}

float CellGame::GameParameters::compute_max_speed_from_mass(float mass) const
{
    return std::max(0.0f, base_cell_speed - speed_loss_factor * mass);
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
    Q_ASSERT(cA + cB != 0); // because dividing by zero creates black holes

    // vect(A->G) = (b/(a+b)) * vect(A->B)
    float c = cB / (cA + cB);

    float dx = b.x - a.x;
    float dy = b.y - a.y;

    return Position(a.x + c*dx,
                    a.y + c*dy);
}
