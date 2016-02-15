#include "visu.hpp"

Visu::Visu()
{
    // initialisation des attributs : pour la plupart il faut attendre un Welcome pour avoir les données

    // création de la fenêtre
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    window.create(sf::VideoMode(1024, 768), "Le jeu des cellules qui se mangent", sf::Style::Default, settings);
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false); // désactiver la répétition des touches si on les maintient appuyées

}

Visu::~Visu()
{
    /*delete window;
    window = nullptr;*/
}

void Visu::onWelcomeReceived(const Welcome &welcome)
{
    // todo
    // enregistrer les paramètres de la partie dans la classe
    parameters.map_width = welcome.parameters.map_width;
    parameters.map_height = welcome.parameters.map_height;
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
        // pb : la cellule va être détruite à la sortie de la fonction, à moins qu'elle ne soit un membre de la classe.

        // l'ajouter dans le conteneur de toutes les cellules
    }

    // initialiser la liste des joueurs
    // besoin de connaître le nombre total de joueurs


}

void Visu::onTurnReceived(const Turn &turn)
{
    // todo
    // mettre à jour les positions de chaque cellule

    // mettre à jour le score des joueurs

    // mettre à jour les sprites et l'affichage
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


    // faire qqch de spécial si la cellule est isolée
    if (cellule->remaining_isolated_turns != 0) {
        cercle.setOutlineColor(sf::Color::Black);
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

