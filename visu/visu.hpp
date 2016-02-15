#pragma once

#include <QWidget>
#include <SFML/Graphics.hpp>
#include <vector>
#include <map>

#include "structures.hpp"
#include "cell.hpp"

class Visu
{
// Méthodes
public:
    explicit Visu();
    ~Visu();

    void onWelcomeReceived(const Welcome &welcome);
    void onTurnReceived(const Turn &turn);

    void afficheCellule(const Cellule *cellule); // dessine une cellule dans la fenêtre graphique
    void afficheToutesCellules();

// Attributs
private:
    GameParameters parameters;
    std::vector<Player> players;

    // sprite cellule joueur
    // sprite cellule neutre

public:
    std::map<quint32, Cellule*> allCells; // toutes les cellules, tous types confondus, par m_id croissant. Les premières sont les initiales neutres
    std::vector<Cellule*> allCellsByMass; // toutes les cellules, triées par masse décroissante. Plus tard on utilisera l'algo timsort pour améliorer les performances https://github.com/gfx/cpp-TimSort

    sf::RenderWindow window; // RenderWindow hérite de Window et contient des fonctions pour le dessin

};

