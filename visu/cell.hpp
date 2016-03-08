#ifndef CELL_H
#define CELL_H

#include <QObject>
#include <SFML/Graphics.hpp>

#include "ainetlib16.hpp"

enum cellType {
    initialNeutral,
    nonInitialNeutral,
    virus,
    player
};

// classe qui peut contenir n'importe quel type de cellule : les neutres (non)intitiales, celles des joueurs et les virus
// la cellule est forcément présente (pas de remaining_turns_before_apparition)
class Cellule
{
// Méthodes
public:
    // constructeurs:
    Cellule(ainet16::TurnPlayerCell cellule, int nbe_players);
    Cellule(ainet16::TurnNonInitialNeutralCell cellule);
    Cellule(ainet16::TurnInitialNeutralCell cellule, float initialNeutralCellsMass, quint32 id);
    Cellule(ainet16::TurnVirus cellule, float virus_mass);
    Cellule(ainet16::Position cellule, float initial_mass, quint32 id);

    void transformToNeutralCell();

    // accesseurs:
    quint32 id() const;
    qint32 player_id() const;


    // autres:
    void print() const; // affichage des attributs en console

// Attributs
private:
    quint32 m_id; // identifiant unique des cellules

public:
    float mass;
    ainet16::Position position;
    cellType typeDeCellule;
    qint32 m_player_id; // vaut -1 si ce n'est pas une cellule de joueur
    quint32 remaining_turns_before_apparition;
    quint32 remaining_isolated_turns; // vaut 0 pour toutes les cellules non joueuses
    bool estVivante;

    sf::Color color; // la couleur dépendra du type de la cellule et du joueur si c'est la cellule d'un joueur
};



#endif // CELL_H
