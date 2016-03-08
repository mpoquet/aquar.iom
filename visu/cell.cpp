#include "cell.hpp"
#include "visu.hpp"
#include <iostream>

Cellule::Cellule(ainet16::TurnPlayerCell cellule, int nbe_players)
{
    m_id = cellule.pcell_id;
    mass = cellule.mass;
    position.x = cellule.position.x;
    position.y = cellule.position.y;
    typeDeCellule = player;
    m_player_id = cellule.player_id;
    remaining_turns_before_apparition = 0;
    remaining_isolated_turns = cellule.remaining_isolated_turns;

    // déterminer la couleur en fonction du numéro du joueur
    color = colorFromPlayerId(m_player_id, nbe_players); // todo: associate the color to the player to avoid O(n) computes instead of O(1)

    estVivante = true;
}

Cellule::Cellule(ainet16::TurnNonInitialNeutralCell cellule) {
    m_id = cellule.ncell_id;
    mass = cellule.mass;
    position.x = cellule.position.x;
    position.y = cellule.position.y;
    typeDeCellule = nonInitialNeutral;
    m_player_id = -1;
    remaining_turns_before_apparition = 0;
    remaining_isolated_turns = 0;

    // même couleur que les cellules neutres initiales : blanc
    color = sf::Color::White;

    estVivante = true;
}

Cellule::Cellule(ainet16::TurnInitialNeutralCell cellule, float initialNeutralCellsMass, quint32 id) {
    // todo : m_id
    m_id = id;
    mass = initialNeutralCellsMass;
    // todo : la position des cellules initiales est dans InitialNeutralCellWelcome
    position.x = 0;
    position.y = 0;
    typeDeCellule = initialNeutral;
    m_player_id = -1;
    remaining_turns_before_apparition = cellule.remaining_turns_before_apparition;
    remaining_isolated_turns = 0;

    // couleur blanche
    color = sf::Color::White;

    estVivante = true;
}

Cellule::Cellule(ainet16::TurnVirus cellule, float virus_mass) {
    m_id = cellule.id;
    mass = virus_mass;
    position.x = cellule.position.x;
    position.y = cellule.position.y;
    typeDeCellule = virus;
    // todo : player id
    m_player_id = -1;
    remaining_turns_before_apparition = 0;
    remaining_isolated_turns = 0;

    // couleur rouge
    color = sf::Color::Black;

    estVivante = true;
}

Cellule::Cellule(ainet16::Position cellule, float initial_mass, quint32 id)
{
    m_id = id;
    mass = initial_mass;
    position.x = cellule.x;
    position.y = cellule.y;
    typeDeCellule = initialNeutral;
    // todo : player id
    m_player_id = -1;
    remaining_turns_before_apparition = 0;
    remaining_isolated_turns = 0;

    // couleur blanche
    color = sf::Color::White;

    estVivante = true;
}

void Cellule::transformToNeutralCell()
{
    m_player_id = -1;
    typeDeCellule = nonInitialNeutral;
    remaining_turns_before_apparition = 0;
    remaining_isolated_turns = 0;

    color = sf::Color::White;
    estVivante = true;
}

quint32 Cellule::id() const
{
    return m_id;
}

qint32 Cellule::player_id() const
{
    return m_player_id;
}

void Cellule::print() const
{
    std::cout << "\nCellule n°" << m_id << std::endl;
    std::cout << "masse : " << mass << std::endl;
    std::cout << "abscisse : " << position.x << " ; ordonnée : " << position.y << std::endl;
    std::cout << "type : " << typeDeCellule << std::endl;
    std::cout << "apparition dans " << remaining_turns_before_apparition << " tours\n";
    if (typeDeCellule == player) {
        std::cout << "appartient au joueur " << m_player_id << std::endl;
        if (remaining_isolated_turns != 0) {
            std::cout << "isolée pour " << remaining_isolated_turns << " tours\n";
        }
    }

}

