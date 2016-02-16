#include "cell.hpp"
#include "visu.hpp"
#include <iostream>

Cellule::Cellule(PlayerCell cellule)
{
    m_id = cellule.id;
    mass = cellule.mass;
    position.x = cellule.position.x;
    position.y = cellule.position.y;
    typeDeCellule = player;
    player_id = cellule.player_id;
    remaining_turns_before_apparition = 0;
    remaining_isolated_turns = cellule.remaining_isolated_turns;

    // déterminer la couleur en fonction du numéro du joueur
    color = colorFromPlayerId(player_id);

    estVivante = true;
}

Cellule::Cellule(NonInitialNeutralCell cellule) {
    m_id = cellule.id;
    mass = cellule.mass;
    position.x = cellule.position.x;
    position.y = cellule.position.y;
    typeDeCellule = nonInitialNeutral;
    player_id = -1;
    remaining_turns_before_apparition = 0;
    remaining_isolated_turns = 0;

    // même couleur que les cellules neutres initiales : blanc
    color = sf::Color::White;

    estVivante = true;
}

Cellule::Cellule(InitialNeutralCell cellule, float initialNeutralCellsMass, quint32 id) {
    // todo : m_id
    m_id = id;
    mass = initialNeutralCellsMass;
    // todo : la position des cellules initiales est dans InitialNeutralCellWelcome
    position.x = 0;
    position.y = 0;
    typeDeCellule = initialNeutral;
    player_id = -1;
    remaining_turns_before_apparition = cellule.remaining_turns_before_apparition;
    remaining_isolated_turns = 0;

    // couleur blanche
    color = sf::Color::White;

    estVivante = true;
}

Cellule::Cellule(Virus cellule, float virus_mass) {
    m_id = cellule.id;
    mass = virus_mass;
    position.x = cellule.position.x;
    position.y = cellule.position.y;
    typeDeCellule = virus;
    // todo : player id
    player_id = -1;
    remaining_turns_before_apparition = 0;
    remaining_isolated_turns = 0;

    // couleur rouge
    color = sf::Color::Red;

    estVivante = true;
}

Cellule::Cellule(InitialNeutralCellWelcome cellule, float initial_mass, quint32 id)
{
    m_id = id;
    mass = initial_mass;
    position.x = cellule.position.x;
    position.y = cellule.position.y;
    typeDeCellule = initialNeutral;
    // todo : player id
    player_id = -1;
    remaining_turns_before_apparition = 0;
    remaining_isolated_turns = 0;

    // couleur blanche
    color = sf::Color::White;

    estVivante = true;
}

quint32 Cellule::id() const
{
    return m_id;
}

void Cellule::print() const
{
    std::cout << "\nCellule n°" << m_id << std::endl;
    std::cout << "masse : " << mass << std::endl;
    std::cout << "abscisse : " << position.x << " ; ordonnée : " << position.y << std::endl;
    std::cout << "type : " << typeDeCellule << std::endl;
    std::cout << "apparition dans " << remaining_turns_before_apparition << " tours\n";
    if (typeDeCellule == player) {
        std::cout << "appartient au joueur " << player_id << std::endl;
        if (remaining_isolated_turns != 0) {
            std::cout << "isolée pour " << remaining_isolated_turns << " tours\n";
        }
    }

}

