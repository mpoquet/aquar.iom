#include "visu.hpp"

Visu::Visu()
{
    // initialisation des attributs : pour la plupart il faut attendre un Welcome pour avoir les données
    window_height = 768;
    window_width = 1024;

    // création de la fenêtre
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    window.create(sf::VideoMode(window_width, window_height), "Le jeu des cellules qui se mangent", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false); // désactiver la répétition des touches si on les maintient appuyées

}

Visu::~Visu()
{
}

void Visu::onWelcomeReceived(const Welcome &welcome)
{
    // enregistrer les paramètres de la partie dans la classe
    parameters.map_width = welcome.parameters.map_width;
    parameters.map_height = welcome.parameters.map_height;

    parameters.min_nb_players = welcome.parameters.min_nb_players;
    parameters.max_nb_players = welcome.parameters.max_nb_players;

    parameters.mass_absorption = welcome.parameters.mass_absorption;
    parameters.minimum_mass_ratio_to_absorb = welcome.parameters.minimum_mass_ratio_to_absorb;
    parameters.minimum_pcell_mass = welcome.parameters.minimum_pcell_mass;
    parameters.radius_factor = welcome.parameters.radius_factor;
    parameters.max_cells_per_player = welcome.parameters.max_cells_per_player;
    parameters.mass_loss_per_frame = welcome.parameters.mass_loss_per_frame; // not useful for visu
    parameters.base_cell_speed = welcome.parameters.base_cell_speed; // not useful for visu
    parameters.speed_loss_factor = welcome.parameters.speed_loss_factor; // not useful for visu
    parameters.virus_mass = welcome.parameters.virus_mass; // tous les virus ont la même masse
    parameters.virus_creation_mass_loss = welcome.parameters.virus_creation_mass_loss; // not useful for visu
    parameters.virus_max_split = welcome.parameters.virus_max_split; // not useful for visu
    parameters.nb_starting_cells_per_player = welcome.parameters.nb_starting_cells_per_player; // not useful for visu
    parameters.player_cells_starting_mass = welcome.parameters.player_cells_starting_mass; // not useful for visu
    parameters.initial_neutral_cells_mass = welcome.parameters.initial_neutral_cells_mass;
    parameters.neutral_cells_repop_time = welcome.parameters.neutral_cells_repop_time;

    // initialiser l'ensemble des cellules avec leurs positions initiales
    QVector<InitialNeutralCellWelcome>::const_iterator it;
    for (it=welcome.initial_ncells.begin() ; it!=welcome.initial_ncells.end() ; ++it) {
        // créer la cellule
        //Cellule* cell = new Cellule(*it, parameters.initial_neutral_cells_mass);

        // l'ajouter dans le conteneur de toutes les cellules
        //addNewCell(cell);
    }

    // initialiser la liste des joueurs
    // pas besoin de le faire maintenant : on va les recevoir dans onTurnReceived

    cadre.setSize(sf::Vector2f(parameters.map_width, parameters.map_height));
    cadre.setFillColor(sf::Color::White);
    cadre.setOutlineColor(sf::Color::Black);
    cadre.setOutlineThickness(1);

}

void Visu::onTurnReceived(const Turn &turn)
{
    // todo
    // mettre à jour les positions de chaque cellule

    // mettre à jour le score des joueurs

    // mettre à jour les sprites et l'affichage

    // faire l'affichage et la mise à jour lors d'un seul parcours de l'ensemble des cellules
}

void Visu::afficheCellule(const Cellule* cellule)
{
    if (cellule->remaining_turns_before_apparition != 0) {
        return; // La cellule n'est pas encore apparue donc on ne l'affiche pas
    }

    float rayon = cellule->mass * parameters.radius_factor;
    sf::CircleShape cercle(rayon, 128);
    cercle.setFillColor(cellule->color);
    cercle.setPosition(cellule->position.x - rayon, cellule->position.y - rayon); // setPosition prend la position du coin supérieur gauche =(

    // mettre un contour pour les cellules isolées et les cellules neutres
    if (cellule->remaining_isolated_turns != 0 | cellule->typeDeCellule == initialNeutral | cellule->typeDeCellule == nonInitialNeutral) {
        cercle.setOutlineColor(sf::Color(100, 100, 100));
        cercle.setOutlineThickness(rayon/8);
    }
    window.draw(cercle);
}

void Visu::afficheToutesCellules()
{
    std::vector<Cellule*>::iterator it;
    for (it=allCellsByMass.begin() ; it!=allCellsByMass.end() ; ++it) {
        afficheCellule(*it);
    }
}

void Visu::afficheScore()
{
    // trier les joueurs par score décroissant
    sort(players.begin(), players.end(), CompareJoueurs());

    // rectangles pour la représentation du score des joueurs
    sf::RectangleShape rect; // rectangle de dimensions (0,0)
    float hauteur(1.0/3.0 * (window.getSize().y - parameters.map_height)), largeur(0);
    float abscisse(0), ordonnee(parameters.map_height + hauteur);

    std::vector<Player>::iterator joueur;
    float somme_scores(0);
    for (joueur=players.begin(); joueur!=players.end(); ++joueur) {
        somme_scores += (*joueur).score;
    }
    float facteur = window.getSize().x / somme_scores;

    for (joueur=players.begin(); joueur!=players.end(); ++joueur) {
        largeur = (*joueur).score * facteur;
        //std::cout << largeur << " " << hauteur << std::endl;
        rect.setSize(sf::Vector2f(largeur, hauteur));
        rect.setFillColor(colorFromPlayerId((*joueur).id, players.size()));

        rect.setPosition(sf::Vector2f(abscisse,ordonnee));
        abscisse += largeur; // on décale le rectangle pour afficher tous les rectangles côte à côte

        window.draw(rect);
        std::cout << (*joueur).id  << " " << (*joueur).score << std::endl;
    }


}

void Visu::addNewCell(Cellule *cellule)
{
    allCells[cellule->id()] = cellule;
}



sf::Color colorFromPlayerId(quint32 playerId, int nbPlayers)
{
    //Il ne faut pas que green et blue soient nuls en même temps car ça fait du rouge et la couleur est déjà prise par les virus.
    sf::Uint8 red = cos(2*playerId)*255.0;
    sf::Uint8 green = sin(7.2*playerId+1.89)*255.0;
    sf::Uint8 blue = tan(8*playerId);
    return sf::Color(red, green, blue);
}
