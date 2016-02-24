#include "cell_game.hpp"

#include <QVector2D>
#include <QQueue>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QString>
#include <QStringList>

#include <cmath>

#include "server.hpp"
#include "client.hpp"

using namespace std;

CellGame::CellGame()
{
    connect(&_timer, &QTimer::timeout, this, &CellGame::onTurnEnd);
}

CellGame::~CellGame()
{

}

void CellGame::onPlayerConnected(Client *client)
{
    Game::onPlayerConnected(client);

    int player_id = _playerClients.indexOf({client,true});

    // If the player has been accepted
    if (player_id != -1)
    {
        if (_playerClients.size() > (int)_parameters.max_nb_players)
        {
            _playerClients.remove(player_id);
            emit message("A player has been accepted whereas the maximum number of players has been reached");
            emit wantToKick(client, "Maximum number of players reached");
        }
        else
        {
            Q_ASSERT(_server != nullptr);
            connect(this, &Game::wantToSendGameStarts, _server, &Server::sendGameStarts);
            connect(this, &Game::wantToSendWelcome, _server, &Server::sendWelcome);
            connect(this, &Game::wantToSendTurn, _server, &Server::sendTurn);
            connect(this, &Game::wantToSendGameEnds, _server, &Server::sendGameEnds);

            QByteArray welcome_message = generate_welcome();
            emit wantToSendWelcome(client, welcome_message);
        }
    }
}

void CellGame::onPlayerMove(Client *client, int turn, QByteArray data)
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

void CellGame::onVisuAck(Client *client, int turn, QByteArray data)
{
    (void) client;
    (void) turn;
    (void) data;
}

void CellGame::onStart()
{
    if (_isRunning)
    {
        emit message("Cannot start game: it is already running...");
        return;
    }

    if (_playerClients.size() < 2)
    {
        emit message("Cannot start game: there must be at least 2 connected players");
        return;
    }

    if (!_parameters.is_loaded)
    {
        emit message("Cannot start game: parameters have not been loaded");
        return;
    }

    // Send GAME_STARTS to all players with the first TURN
    int player_id = 0;
    for (const GameClient & client : _playerClients)
    {
        if (client.connected)
        {
            QByteArray message = generate_game_starts(player_id);
            emit wantToSendGameStarts(client.client, message);
        }

        ++player_id;
    }

    QByteArray message = generate_game_starts(42);
    for (const GameClient & client : _visuClients)
    {
        if (client.connected)
            emit wantToSendGameStarts(client.client, message);
    }

    _timer.start();
    _isRunning = true;
}

void CellGame::onPause()
{
    emit message("Pausing this game is not implemented yet :)");
}

void CellGame::onResume()
{
    emit message("Resuming this game is not implemented yet :)");
}

void CellGame::onStop()
{
    emit message("Stopping this game is not implemented yet :)");
}

void CellGame::onTurnTimerChanged(quint32 ms)
{
    _timer.setInterval(ms);
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
    send_turn_to_everyone();
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
        if (action->new_cell_mass < _parameters.minimum_player_cell_mass)
        {
            delete action;
            continue;
        }

        // If the cell is not big enough to be split, the action is ignored
        if (cell->mass < _parameters.minimum_player_cell_mass * 2)
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
                ncell->remaining_turns_before_apparition = _parameters.initial_neutral_cells_repop_time;
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
            Q_ASSERT(total_mass / 2 >= _parameters.minimum_player_cell_mass);
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
                Q_ASSERT(mass_by_satellite >= _parameters.minimum_player_cell_mass);

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

void CellGame::load_parameters(const QString &filename)
{
    if (_isRunning)
    {
        emit message("Cannot change game parameters while the game is running");
        return;
    }

    _parameters.clear();
    emit message("Loading parameters...");

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        emit message("Cannot open file " + filename);
        return;
    }

    QString file_content = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(file_content.toUtf8());

    if (!doc.isObject())
    {
        emit message(QString("Invalid file '%1': not a JSON object").arg(filename));
        return;
    }














    if (!doc.object().contains("map_width")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'map_width' key").arg(filename));
        return;
    }
    else if (!doc.object()["map_width"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'map_width' is not a number").arg(filename));
        return;
    }
    _parameters.map_width = doc.object()["map_width"].toDouble();

    if (!doc.object().contains("map_height")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'map_height' key").arg(filename));
        return;
    }
    else if (!doc.object()["map_height"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'map_height' is not a number").arg(filename));
        return;
    }
    _parameters.map_height = doc.object()["map_height"].toDouble();

    if (!doc.object().contains("min_nb_players")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'min_nb_players' key").arg(filename));
        return;
    }
    else if (!doc.object()["min_nb_players"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'min_nb_players' is not a number").arg(filename));
        return;
    }
    _parameters.min_nb_players = doc.object()["min_nb_players"].toInt();

    if (!doc.object().contains("max_nb_players")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'max_nb_players' key").arg(filename));
        return;
    }
    else if (!doc.object()["max_nb_players"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'max_nb_players' is not a number").arg(filename));
        return;
    }
    _parameters.max_nb_players = doc.object()["max_nb_players"].toInt();

    if (!doc.object().contains("nb_starting_cells_per_player")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'nb_starting_cells_per_player' key").arg(filename));
        return;
    }
    else if (!doc.object()["nb_starting_cells_per_player"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'nb_starting_cells_per_player' is not a number").arg(filename));
        return;
    }
    _parameters.nb_starting_cells_per_player = doc.object()["nb_starting_cells_per_player"].toInt();

    if (!doc.object().contains("players_starting_positions")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'players_starting_positions' key").arg(filename));
        return;
    }
    else if (!doc.object()["players_starting_positions"].isArray()) {
        emit message(QString("Invalid file '%1': the value associated to key 'players_starting_positions' is not an array").arg(filename));
        return;
    }
    QJsonArray array_ps_positions = doc.object()["players_starting_positions"].toArray();
    _parameters.players_starting_positions.resize(array_ps_positions.size());
    for (int i = 0; i < array_ps_positions.size(); ++i)
    {
        if (!array_ps_positions[i].isArray()) {
            emit message(QString("Invalid file '%1': the 'players_starting_positions' array subelements must be arrays of size 2 but value %2 isn't").arg(filename, i));
            return;
        }
        QJsonArray subarray = array_ps_positions[i].toArray();
        if (subarray.size() != 2) {
            emit message(QString("Invalid file '%1': the 'players_starting_positions' array subelements must be arrays of size 2 but value %2 isn't").arg(filename, i));
            return;
        }
        QJsonValue posx = subarray[0];
        QJsonValue posy = subarray[1];

        if (!posx.isDouble() || !posy.isDouble()) {
            emit message(QString("Invalid file '%1': the 'players_starting_positions' array subelements must be arrays of size 2 of double values but the values of subarray %2 are not doubles").arg(filename, i));
            return;
        }

        _parameters.players_starting_positions[i].x = posx.toDouble();
        _parameters.players_starting_positions[i].y = posy.toDouble();
    }

    if (!doc.object().contains("mass_absorption")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'mass_absorption' key").arg(filename));
        return;
    }
    else if (!doc.object()["mass_absorption"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'mass_absorption' is not a number").arg(filename));
        return;
    }
    _parameters.mass_absorption = doc.object()["mass_absorption"].toDouble();

    if (!doc.object().contains("minimum_mass_ratio_to_absorb")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'minimum_mass_ratio_to_absorb' key").arg(filename));
        return;
    }
    else if (!doc.object()["minimum_mass_ratio_to_absorb"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'minimum_mass_ratio_to_absorb' is not a number").arg(filename));
        return;
    }
    _parameters.minimum_mass_ratio_to_absorb = doc.object()["minimum_mass_ratio_to_absorb"].toDouble();

    if (!doc.object().contains("radius_factor")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'radius_factor' key").arg(filename));
        return;
    }
    else if (!doc.object()["radius_factor"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'radius_factor' is not a number").arg(filename));
        return;
    }
    _parameters.radius_factor = doc.object()["radius_factor"].toDouble();

    if (!doc.object().contains("player_cells_starting_mass")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'player_cells_starting_mass' key").arg(filename));
        return;
    }
    else if (!doc.object()["player_cells_starting_mass"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'player_cells_starting_mass' is not a number").arg(filename));
        return;
    }
    _parameters.player_cells_starting_mass = doc.object()["player_cells_starting_mass"].toDouble();

    if (!doc.object().contains("max_cells_per_player")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'max_cells_per_player' key").arg(filename));
        return;
    }
    else if (!doc.object()["max_cells_per_player"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'max_cells_per_player' is not a number").arg(filename));
        return;
    }
    _parameters.max_cells_per_player = doc.object()["max_cells_per_player"].toInt();

    if (!doc.object().contains("mass_loss_per_frame")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'mass_loss_per_frame' key").arg(filename));
        return;
    }
    else if (!doc.object()["mass_loss_per_frame"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'mass_loss_per_frame' is not a number").arg(filename));
        return;
    }
    _parameters.mass_loss_per_frame = doc.object()["mass_loss_per_frame"].toDouble();

    if (!doc.object().contains("base_cell_speed")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'base_cell_speed' key").arg(filename));
        return;
    }
    else if (!doc.object()["base_cell_speed"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'base_cell_speed' is not a number").arg(filename));
        return;
    }
    _parameters.base_cell_speed = doc.object()["base_cell_speed"].toDouble();

    if (!doc.object().contains("speed_loss_factor")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'speed_loss_factor' key").arg(filename));
        return;
    }
    else if (!doc.object()["speed_loss_factor"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'speed_loss_factor' is not a number").arg(filename));
        return;
    }
    _parameters.speed_loss_factor = doc.object()["speed_loss_factor"].toDouble();

    if (!doc.object().contains("minimum_player_cell_mass")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'minimum_player_cell_mass' key").arg(filename));
        return;
    }
    else if (!doc.object()["minimum_player_cell_mass"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'minimum_player_cell_mass' is not a number").arg(filename));
        return;
    }
    _parameters.minimum_player_cell_mass = doc.object()["minimum_player_cell_mass"].toDouble();

    if (!doc.object().contains("initial_neutral_cells_matrix_width")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'initial_neutral_cells_matrix_width' key").arg(filename));
        return;
    }
    else if (!doc.object()["initial_neutral_cells_matrix_width"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'initial_neutral_cells_matrix_width' is not a number").arg(filename));
        return;
    }
    _parameters.initial_neutral_cells_matrix_width = doc.object()["initial_neutral_cells_matrix_width"].toInt();

    if (!doc.object().contains("initial_neutral_cells_matrix_height")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'initial_neutral_cells_matrix_height' key").arg(filename));
        return;
    }
    else if (!doc.object()["initial_neutral_cells_matrix_height"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'initial_neutral_cells_matrix_height' is not a number").arg(filename));
        return;
    }
    _parameters.initial_neutral_cells_matrix_height = doc.object()["initial_neutral_cells_matrix_height"].toInt();

    if (!doc.object().contains("initial_neutral_cells_mass")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'initial_neutral_cells_mass' key").arg(filename));
        return;
    }
    else if (!doc.object()["initial_neutral_cells_mass"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'initial_neutral_cells_mass' is not a number").arg(filename));
        return;
    }
    _parameters.initial_neutral_cells_mass = doc.object()["initial_neutral_cells_mass"].toDouble();

    if (!doc.object().contains("initial_neutral_cells_repop_time")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'initial_neutral_cells_repop_time' key").arg(filename));
        return;
    }
    else if (!doc.object()["initial_neutral_cells_repop_time"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'initial_neutral_cells_repop_time' is not a number").arg(filename));
        return;
    }
    _parameters.initial_neutral_cells_repop_time = doc.object()["initial_neutral_cells_repop_time"].toInt();

    if (!doc.object().contains("max_viruses")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'max_viruses' key").arg(filename));
        return;
    }
    else if (!doc.object()["max_viruses"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'max_viruses' is not a number").arg(filename));
        return;
    }
    _parameters.max_viruses = doc.object()["max_viruses"].toInt();

    if (!doc.object().contains("virus_mass")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'virus_mass' key").arg(filename));
        return;
    }
    else if (!doc.object()["virus_mass"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'virus_mass' is not a number").arg(filename));
        return;
    }
    _parameters.virus_mass = doc.object()["virus_mass"].toDouble();

    if (!doc.object().contains("virus_creation_mass_loss")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'virus_creation_mass_loss' key").arg(filename));
        return;
    }
    else if (!doc.object()["virus_creation_mass_loss"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'virus_creation_mass_loss' is not a number").arg(filename));
        return;
    }
    _parameters.virus_creation_mass_loss = doc.object()["virus_creation_mass_loss"].toDouble();

    if (!doc.object().contains("virus_max_split")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'virus_max_split' key").arg(filename));
        return;
    }
    else if (!doc.object()["virus_max_split"].isDouble()) {
        emit message(QString("Invalid file '%1': the value associated to key 'virus_max_split' is not a number").arg(filename));
        return;
    }
    _parameters.virus_max_split = doc.object()["virus_max_split"].toInt();

    if (!doc.object().contains("viruses_starting_positions")) {
        emit message(QString("Invalid file '%1': the root object does not contain the 'viruses_starting_positions' key").arg(filename));
        return;
    }
    else if (!doc.object()["viruses_starting_positions"].isArray()) {
        emit message(QString("Invalid file '%1': the value associated to key 'viruses_starting_positions' is not an array").arg(filename));
        return;
    }
    // tofix _parameters.viruses_starting_positions = doc.object()["viruses_starting_positions"].toDouble();
    QJsonArray array_vs_positions = doc.object()["viruses_starting_positions"].toArray();
    _parameters.viruses_starting_positions.resize(array_vs_positions.size());
    for (int i = 0; i < array_vs_positions.size(); ++i)
    {
        if (!array_vs_positions[i].isArray()) {
            emit message(QString("Invalid file '%1': the 'viruses_starting_positions' array subelements must be arrays of size 2 but value %2 isn't").arg(filename, i));
            return;
        }
        QJsonArray subarray = array_vs_positions[i].toArray();
        if (subarray.size() != 2) {
            emit message(QString("Invalid file '%1': the 'viruses_starting_positions' array subelements must be arrays of size 2 but value %2 isn't").arg(filename, i));
            return;
        }
        QJsonValue posx = subarray[0];
        QJsonValue posy = subarray[1];

        if (!posx.isDouble() || !posy.isDouble()) {
            emit message(QString("Invalid file '%1': the 'viruses_starting_positions' array subelements must be arrays of size 2 of double values but the values of subarray %2 are not doubles").arg(filename, i));
            return;
        }

        _parameters.viruses_starting_positions[i].x = posx.toDouble();
        _parameters.viruses_starting_positions[i].y = posy.toDouble();
    }













    emit message("Checking parameters...");

    QString reason;
    if (!_parameters.is_valid(reason))
    {
        emit message(QString("The set of parameters given in file '%1' is invalid:\n%2").arg(filename, reason));
        return;
    }

    _parameters.nb_initial_neutral_cells = _parameters.initial_neutral_cells_matrix_width *
                                           _parameters.initial_neutral_cells_matrix_height;
    _parameters.is_loaded = true;

    Q_ASSERT(_server != nullptr);
    _server->setServerMaxPlayers(_parameters.max_nb_players);

    emit message("Parameters have been loaded");
}

void CellGame::make_player_repop(CellGame::Player *player)
{
    // todo
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
    mass = std::max(new_mass, parameters.minimum_player_cell_mass);

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

void CellGame::GameParameters::clear()
{
    is_loaded = false;

    map_width = -1;
    map_height = -1;

    mass_absorption = -1;
    minimum_mass_ratio_to_absorb = -1;
    minimum_player_cell_mass = -1;

    radius_factor = -1;
    max_cells_per_player = -1;
    mass_loss_per_frame = -1;

    base_cell_speed = -1;
    speed_loss_factor = -1;

    initial_neutral_cells_matrix_width = -1;
    initial_neutral_cells_matrix_height = -1;
    nb_initial_neutral_cells = -1;
    initial_neutral_cells_mass = -1;
    initial_neutral_cells_repop_time = -1;

    max_viruses = -1;
    virus_mass = -1;
    virus_creation_mass_loss = -1;
    virus_max_split = -1;

    min_nb_players = -1;
    max_nb_players = -1;
    nb_starting_cells_per_player = -1;
    player_cells_starting_mass = -1;
    players_starting_positions.clear();

    viruses_starting_positions.clear();
}

bool CellGame::GameParameters::is_valid(QString &invalidity_reason) const
{
    bool ret = true;

    if (map_width <= 0)
    {
        ret = false;
        invalidity_reason += "map_width must be > 0\n";
    }

    if (map_height <= 0)
    {
        ret = false;
        invalidity_reason += "map_height must be > 0\n";
    }

    if (mass_absorption <= 0 || mass_absorption > 2)
    {
        ret = false;
        invalidity_reason += "mass_absorption must be in ]0,2]\n";
    }

    if (minimum_mass_ratio_to_absorb <= 1 || minimum_mass_ratio_to_absorb > 3)
    {
        ret = false;
        invalidity_reason += "minimum_mass_ratio_to_absorb must be in ]1,3]\n";
    }

    if (radius_factor <= 0 || radius_factor > 2)
    {
        ret = false;
        invalidity_reason += "radius_factor must be in ]0,2]\n";
    }

    if (max_cells_per_player < 1)
    {
        ret = false;
        invalidity_reason += "max_cells_per_player must be greater than or equal to 1\n";
    }

    if (mass_loss_per_frame < 0 || mass_loss_per_frame >= 1)
    {
        ret = false;
        invalidity_reason += "mass_loss_per_frame must be in [0,1[\n";
    }

    if (base_cell_speed <= 0 || base_cell_speed > 1000)
    {
        ret = false;
        invalidity_reason += "base_cell_speed must be in ]0,1000]\n";
    }

    if (speed_loss_factor < 0 || speed_loss_factor > 100)
    {
        ret = false;
        invalidity_reason += "speed_loss_factor must be in [0,100]\n";
    }

    if (initial_neutral_cells_matrix_width < 1)
    {
        ret = false;
        invalidity_reason += "initial_neutral_cells_matrix_width must be greater than or equal to 1\n";
    }

    if (initial_neutral_cells_matrix_height < 1)
    {
        ret = false;
        invalidity_reason += "initial_neutral_cells_matrix_height must be greater than or equal to 1\n";
    }

    if (initial_neutral_cells_mass < 0)
    {
        ret = false;
        invalidity_reason += "initial_neutral_cells_mass must be greater than or equal to 0\n";
    }

    if (initial_neutral_cells_repop_time < 1)
    {
        ret = false;
        invalidity_reason += "initial_neutral_cells_repop_time must be greater than or equal to 1\n";
    }

    if (max_viruses > 128)
    {
        ret = false;
        invalidity_reason += "max_viruses must be in [0,128]\n";
    }

    if (virus_mass < 0)
    {
        ret = false;
        invalidity_reason += "virus_mass must be greater than or equal to 0\n";
    }

    if (virus_creation_mass_loss < 0 || virus_creation_mass_loss >= 1)
    {
        ret = false;
        invalidity_reason += "virus_creation_mass_loss must be in [0,1[\n";
    }

    if (virus_max_split > 64)
    {
        ret = false;
        invalidity_reason += "virus_max_split must be in [0,64]\n";
    }

    if (min_nb_players < 1)
    {
        ret = false;
        invalidity_reason += "min_nb_players must be greater than or equal to 1\n";
    }

    if (max_nb_players < 1)
    {
        ret = false;
        invalidity_reason += "max_nb_players must be greater than or equal to 1\n";
    }

    if (min_nb_players > max_nb_players)
    {
        ret = false;
        invalidity_reason += "min_nb_players must be lesser than or equal to max_nb_players\n";
    }

    if (nb_starting_cells_per_player < 1)
    {
        ret = false;
        invalidity_reason += "nb_starting_cells_per_player must be greater than or equal to 1\n";
    }

    if (nb_starting_cells_per_player > max_cells_per_player)
    {
        ret = false;
        invalidity_reason += "nb_starting_cells_per_player cannot exceed max_cells_per_player\n";
    }

    if (player_cells_starting_mass <= initial_neutral_cells_mass * minimum_mass_ratio_to_absorb)
    {
        ret = false;
        invalidity_reason += "player_cells_starting_mass must be greater than initial_neutral_cells_mass * minimum_mass_ratio_to_absorb to absorb initial neutral cells\n";
    }

    if ((quint32)players_starting_positions.size() != nb_starting_cells_per_player * max_nb_players)
    {
        ret = false;
        invalidity_reason += "players_starting_positions must contain nb_starting_cells_per_player * max_nb_players positions\n";
    }

    for (const Position & p : players_starting_positions)
    {
        if (p.x < 0 || p.y >= map_width)
        {
            ret = false;
            invalidity_reason += QString("players_starting_positions is invalid: coordinate %1 is not in [0,map_width=%2[\n").arg(p.x, map_width);
        }

        if (p.y < 0 || p.y >= map_height)
        {
            ret = false;
            invalidity_reason += QString("players_starting_positions is invalid: coordinate %1 is not in [0,map_height=%2[\n").arg(p.y, map_height);
        }
    }

    for (const Position & p : viruses_starting_positions)
    {
        if (p.x < 0 || p.y >= map_width)
        {
            ret = false;
            invalidity_reason += QString("viruses_starting_positions is invalid: coordinate %1 is not in [0,map_width=%2[\n").arg(p.x, map_width);
        }

        if (p.y < 0 || p.y >= map_height)
        {
            ret = false;
            invalidity_reason += QString("viruses_starting_positions is invalid: coordinate %1 is not in [0,map_height=%2[\n").arg(p.y, map_height);
        }
    }

    return ret;
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

QByteArray CellGame::generate_turn()
{
    QByteArray qba, message;

    // Append Initial_neutral_cells
    qba.resize(sizeof(quint32));
    (*(quint32*)qba.data()) = _parameters.nb_initial_neutral_cells;
    message.append(qba);

    QMapIterator<int, NeutralCell*> it_initial_ncells(_initial_neutral_cells);
    while (it_initial_ncells.hasNext())
    {
        it_initial_ncells.next();

        qba.resize(sizeof(quint32));
        (*(quint32*)qba.data()) = it_initial_ncells.value()->remaining_turns_before_apparition;
        message.append(qba);
    }

    // Append Non_initial_neutral_cells
    QByteArray non_initial_ncells_qba;

    QMapIterator<int, NeutralCell*> it_alive_ncells(_alive_neutral_cells);
    while (it_alive_ncells.hasNext())
    {
        it_alive_ncells.next();
        NeutralCell * ncell = it_alive_ncells.value();

        if (!ncell->is_initial)
        {
            qba.resize(sizeof(quint32));
            (*(quint32*)qba.data()) = ncell->id;
            non_initial_ncells_qba.append(qba);

            qba.resize(sizeof(float));
            (*(float*)qba.data()) = ncell->mass;
            non_initial_ncells_qba.append(qba);

            qba.resize(sizeof(float));
            (*(float*)qba.data()) = ncell->position.x;
            non_initial_ncells_qba.append(qba);

            qba.resize(sizeof(float));
            (*(float*)qba.data()) = ncell->position.y;
            non_initial_ncells_qba.append(qba);
        }
    }

    quint32 nb_non_initial_ncells = non_initial_ncells_qba.size() / (sizeof(quint32) + sizeof(float) + sizeof(float)*2);

    qba.resize(sizeof(quint32));
    (*(quint32*)qba.data()) = nb_non_initial_ncells;
    message.append(qba);
    message.append(non_initial_ncells_qba);

    // Append viruses
    qba.resize(sizeof(quint32));
    (*(quint32*)qba.data()) = _viruses.size();
    message.append(qba);

    QMapIterator<int, Virus*> it_viruses(_viruses);
    while (it_viruses.hasNext())
    {
        it_viruses.next();
        Virus * virus = it_viruses.value();

        qba.resize(sizeof(quint32));
        (*(quint32*)qba.data()) = virus->id;
        message.append(qba);

        qba.resize(sizeof(float));
        (*(float*)qba.data()) = virus->position.x;
        message.append(qba);

        qba.resize(sizeof(float));
        (*(float*)qba.data()) = virus->position.y;
        message.append(qba);
    }

    // Append player cells
    qba.resize(sizeof(quint32));
    (*(quint32*)qba.data()) = _player_cells.size();
    message.append(qba);

    QMapIterator<int, PlayerCell*> it_pcells(_player_cells);
    while (it_pcells.hasNext())
    {
        it_pcells.next();
        PlayerCell * pcell = it_pcells.value();

        qba.resize(sizeof(quint32));
        (*(quint32*)qba.data()) = pcell->id;
        message.append(qba);

        qba.resize(sizeof(float));
        (*(float*)qba.data()) = pcell->position.x;
        message.append(qba);

        qba.resize(sizeof(float));
        (*(float*)qba.data()) = pcell->position.y;
        message.append(qba);

        qba.resize(sizeof(quint32));
        (*(quint32*)qba.data()) = pcell->player_id;
        message.append(qba);

        qba.resize(sizeof(float));
        (*(float*)qba.data()) = pcell->mass;
        message.append(qba);
    }

    // Append players
    qba.resize(sizeof(quint32));
    (*(quint32*)qba.data()) = (quint32)_players.size();
    message.append(qba);

    QMapIterator<int, Player *> it_players(_players);
    while (it_players.hasNext())
    {
        it_players.next();
        Player * player = it_players.value();

        qba.resize(sizeof(quint32));
        (*(quint32*)qba.data()) = player->id;
        message.append(qba);

        qba.resize(sizeof(quint32));
        (*(quint32*)qba.data()) = player->nb_cells;
        message.append(qba);

        qba.resize(sizeof(float));
        (*(float*)qba.data()) = player->mass;
        message.append(qba);

        qba.resize(sizeof(quint64));
        (*(quint64*)qba.data()) = player->score;
        message.append(qba);
    }

    return message;
}

QByteArray CellGame::generate_welcome()
{
    QByteArray message;
    QByteArray qba;

    // Game parameters
    qba.resize(sizeof(float));
    (*(float*)qba.data()) = _parameters.map_width;
    message.append(qba);

    qba.resize(sizeof(float));
    (*(float*)qba.data()) = _parameters.map_height;
    message.append(qba);

    qba.resize(sizeof(quint32));
    (*(quint32*)qba.data()) = _parameters.min_nb_players;
    message.append(qba);

    qba.resize(sizeof(quint32));
    (*(quint32*)qba.data()) = _parameters.max_nb_players;
    message.append(qba);

    qba.resize(sizeof(float));
    (*(float*)qba.data()) = _parameters.mass_absorption;
    message.append(qba);

    qba.resize(sizeof(float));
    (*(float*)qba.data()) = _parameters.minimum_mass_ratio_to_absorb;
    message.append(qba);

    qba.resize(sizeof(float));
    (*(float*)qba.data()) = _parameters.minimum_player_cell_mass;
    message.append(qba);

    qba.resize(sizeof(float));
    (*(float*)qba.data()) = _parameters.radius_factor;
    message.append(qba);

    qba.resize(sizeof(quint32));
    (*(quint32*)qba.data()) = _parameters.max_cells_per_player;
    message.append(qba);

    qba.resize(sizeof(float));
    (*(float*)qba.data()) = _parameters.mass_loss_per_frame;
    message.append(qba);

    qba.resize(sizeof(float));
    (*(float*)qba.data()) = _parameters.base_cell_speed;
    message.append(qba);

    qba.resize(sizeof(float));
    (*(float*)qba.data()) = _parameters.speed_loss_factor;
    message.append(qba);

    qba.resize(sizeof(float));
    (*(float*)qba.data()) = _parameters.virus_mass;
    message.append(qba);

    qba.resize(sizeof(float));
    (*(float*)qba.data()) = _parameters.virus_creation_mass_loss;
    message.append(qba);

    qba.resize(sizeof(quint32));
    (*(quint32*)qba.data()) = _parameters.virus_max_split;
    message.append(qba);

    qba.resize(sizeof(quint32));
    (*(quint32*)qba.data()) = _parameters.nb_starting_cells_per_player;
    message.append(qba);

    qba.resize(sizeof(float));
    (*(float*)qba.data()) = _parameters.player_cells_starting_mass;
    message.append(qba);

    qba.resize(sizeof(float));
    (*(float*)qba.data()) = _parameters.initial_neutral_cells_mass;
    message.append(qba);

    qba.resize(sizeof(quint32));
    (*(quint32*)qba.data()) = _parameters.initial_neutral_cells_repop_time;
    message.append(qba);

    // Initial neutral cells' positions
    qba.resize(sizeof(quint32));
    (*(quint32*)qba.data()) = (quint32) _initial_neutral_cells.size();
    message.append(qba);

    QMapIterator<int, NeutralCell *> initial_ncell_iterator(_initial_neutral_cells);
    while (initial_ncell_iterator.hasNext())
    {
        initial_ncell_iterator.next();
        const NeutralCell * ncell = initial_ncell_iterator.value();

        qba.resize(sizeof(float));
        (*(float*)qba.data()) = ncell->position.x;
        message.append(qba);

        qba.resize(sizeof(float));
        (*(float*)qba.data()) = ncell->position.y;
        message.append(qba);
    }

    return message;
}

QByteArray CellGame::generate_game_starts(int player_id)
{
    QByteArray message, qba;

    qba.resize(sizeof(quint32));
    (*(quint32*)qba.data()) = player_id;
    message.append(qba);

    qba = generate_turn();
    message.append(qba);

    return message;
}

void CellGame::send_turn_to_everyone()
{
    QByteArray message = generate_turn();

    // The message content is ready, let it be sent to every player & visu
    for (const GameClient & client : _playerClients)
    {
        if (client.connected)
            emit wantToSendTurn(client.client, _current_turn, message);
    }

    for (const GameClient & client : _visuClients)
    {
        if (client.connected)
            emit wantToSendTurn(client.client, _current_turn, message);
    }

/*  Content: (Initial_neutral_cells, Non_initial_neutral_cells, Viruses, Player_cells, Players)

    Initial_neutral_cells: (nb_initial_cells:ui32, (remaining_turns_before_apparition:ui32)*nb_initial_cells)
    Non_initial_neutral_cells: (nb_non_initial_ncells:ui32, (ncell_id:ui32, mass:float32, position:pos)*nb_non_initial_ncells)
    Viruses: (nb_viruses:ui32, (virus_id:ui32, position:pos)*nb_viruses)
    Player_cells: (nb_player_cells:ui32, (pcell_id:ui32, position:pos, player_id:ui32, mass:float32, remaining_isolated_turns:ui32)*nb_player_cells)
    Players: (nb_players:ui32, (player_id:ui32, nb_pcells:ui32, score:ui64)*nb_players)
*/
}
