#pragma once

#include <QtGlobal>
#include <QVector>

/* Les différents types de cellules :
 *  Les cellules neutres ne se déplacent pas et ne peuvent pas manger d'autres cellules ; elles peuvent être mangées par des joueurs.
 *      Leur masse reste constante (contrairement aux joueurs qui perdent de la masse au cours du temps).
 *  Les cellules neutres initiales réapparaissent au bout d'un certain nbe de tours après avoir été mangées.
 *      Leur position est connue au début de la partie.
 *  Les cellules neutres non initiales ne réapparaissent pas. Par exemple quand un joueur a abandonné.
 *  Les virus peuvent être mangés et font exploser la cellule qui les a mangés en petits bouts, dont la somme des masses est inférieure
 *      à sa masse initiale. Ils sont posés par les joueurs mais n'appartiennent à personne. Les petits bouts sont des cellules isolées
 *      pendant un certain temps (elles ne peuvent pas fusionner tout de suite).
 *
 *  Toutes les cellules sont des disques.
 *
 * Afficher le plateau dans la zone principale et les scores dans un bandeau sur le côté.
 *  On peut afficher le score avec un rectangle (camembert 1 dimension).
 *
 * Les cellules plus petites sont affichées derrière les cellules plus grosses.
 */

// Coordonnées du centre de la cellule
struct Position
{
    float x; // abscisse [0;width]
    float y; // ordonnée [0;heigth]
};

struct InitialNeutralCell
{
    quint32 remaining_turns_before_apparition; // vaut 0 si la cellule est déjà présente
};

struct NonInitialNeutralCell
{
    quint32 id; // identifiant unique parmi toutes les cellules (tous types confondus)
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
    quint32 id; // identifiant unique
    quint32 player_id; // numéro du joueur à qui elle appartient
    Position position;
    float mass;
    quint32 remaining_isolated_turns; // vaut 0 si la cellule a le droit de fusionner
};

struct Player
{
    quint32 id;
    quint32 nb_cells;
    float mass; // masse de toutes ses cellules (somme)
    quint64 score; // intégrale de la masse en fonction du temps (depuis le début de la partie)
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
    float mass_absorption; // not useful for visu
    float minimum_mass_ratio_to_absorb; // not useful for visu
    float minimum_pcell_mass; // not useful for visu?
    float radius_factor; // pour déterminer le rayon d'une cellule à partir de la masse (connue)
    quint32 max_cells_per_player; // not useful for visu
    float mass_loss_per_frame; // not useful for visu
    float base_cell_speed; // not useful for visu
    float speed_loss_factor; // not useful for visu
    float virus_mass; // tous les virus ont la même masse
    float virus_creation_mass_loss; // not useful for visu
    float virus_max_split; // not useful for visu
    quint32 nb_starting_cells_per_player; // not useful for visu
    float player_cells_starting_mass; // not useful for visu
    float initial_neutral_cells_mass;
    quint32 neutral_cells_repop_time; // not useful for visu
};

struct InitialNeutralCellWelcome
{
    Position position;
};

// structure envoyée par le serveur au début du jeu
struct Welcome
{
    GameParameters parameters;
    QVector<InitialNeutralCellWelcome> initial_ncells; // les positions des cellules neutres (elles ne se déplacent pas)
};

