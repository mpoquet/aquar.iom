#ifndef CELL_H
#define CELL_H

#include <QObject>
#include "structures.hpp"
#include <SFML/Graphics.hpp>

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
    Cellule(PlayerCell cellule);
    Cellule(NonInitialNeutralCell cellule);
    Cellule(InitialNeutralCell cellule, float initialNeutralCellsMass);
    Cellule(Virus cellule, float virus_mass);

    // accesseurs:
    quint32 id() const;

    // autres:
    void print() const; // affichage des attributs en console

// Attributs
private:
    quint32 m_id; // identifiant unique des cellules

public:
    float mass;
    Position position;
    cellType typeDeCellule;
    qint32 player_id; // vaut -1 si ce n'est pas une cellule de joueur
    quint32 remaining_turns_before_apparition;
    quint32 remaining_isolated_turns; // vaut 0 pour toutes les cellules non joueuses

    sf::Color color; // la couleur dépendra du type de la cellule et du joueur si c'est la cellule d'un joueur
};



#endif // CELL_H
