#pragma once

#include <QtGlobal>
#include <QVector>

struct Position
{
    float x;
    float y;
};

struct InitialNeutralCell
{
    quint32 remaining_turns_before_apparition;
};

struct NonInitialNeutralCell
{
    quint32 id;
    float mass;
    Position position;
};

struct Virus
{
    quint32 id;
    Position position;
};

struct PlayerCell
{
    quint32 id;
    quint32 player_id;
    Position position;
    float mass;
    quint32 remaining_isolated_turns;
};

struct Player
{
    quint32 id;
    quint32 nb_cells;
    quint64 score;
};

struct Turn
{
    QVector<InitialNeutralCell> initial_ncells;
    QVector<NonInitialNeutralCell> non_initial_ncells;
    QVector<Virus> viruses;
    QVector<PlayerCell> pcells;
    QVector<Player> players;
};

struct GameParameters
{
    float map_width;
    float map_height;
    quint32 min_nb_players;
    quint32 max_nb_players;
    float mass_absorption; // not useful for visu
    float minimum_mass_ratio_to_absorb; // not useful for visu
    float minimum_pcell_mass; // not useful for visu?
    float radius_factor;
    quint32 max_cells_per_player; // not useful for visu
    float mass_loss_per_frame; // not useful for visu
    float base_cell_speed; // not useful for visu
    float speed_loss_factor; // not useful for visu
    float virus_mass;
    float virus_creation_mass_loss; // not useful for visu
    float virus_max_split; // not useful for visu
    quint32 nb_starting_cells_per_player; // not useful for visu
    float player_cells_starting_mass; // not useful for visu
    float neutral_cells_mass;
    quint32 neutral_cells_repop_time; // not useful for visu
};

struct InitialNeutralCellWelcome
{
    Position position;
};

struct Welcome
{
    GameParameters parameters;
    QVector<InitialNeutralCellWelcome> initial_ncells;
};

