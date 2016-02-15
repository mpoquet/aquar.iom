#pragma once

#include <QWidget>
#include <SFML/Graphics.hpp>
#include <vector>
#include <map>
#include <iostream>
#include <tgmath.h>
#include <string>

#include "structures.hpp"
#include "cell.hpp"

class CompareJoueurs {
public:
    bool operator()(const Player a, const Player b) {
        return a.score > b.score;
    }
};


class Visu
{
// Méthodes
public:
    explicit Visu();
    ~Visu();

    void onWelcomeReceived(const Welcome &welcome);
    void onTurnReceived(const Turn &turn);

    // Affiche une cellule dans la fenêtre
    void afficheCellule(const Cellule *cellule);
    // Affiche toutes les cellules
    void afficheToutesCellules();
    // Affiche les scores des joueurs à droite de la fenêtre et la répartition des scores en bas de la fenêtre
    void afficheScore();
    // Affiche le cadre qui délimite le plateau de jeu
    void afficheCadre();

    // Ajoute une cellule dans le conteneur allCells
    void addNewCell(Cellule* cellule);

    // Fait changer la couleur de l'arrière-plan entre blanc et noir
    void inverseCouleurs();

// Attributs
private:
    GameParameters parameters;
    int window_height;
    int window_width;
    sf::Color background_color;
    sf::Color borders_color;

public:
    std::vector<Player> players;
    std::map<quint32, Cellule*> allCells; // toutes les cellules, tous types confondus, par m_id croissant. Les premières sont les initiales neutres
    std::vector<Cellule*> allCellsByMass; // toutes les cellules, triées par masse décroissante. Plus tard on utilisera l'algo timsort pour améliorer les performances https://github.com/gfx/cpp-TimSort

    sf::RenderWindow window; // RenderWindow hérite de Window et contient des fonctions pour le dessin
    sf::RectangleShape cadre; // délimite le plateau de jeu

};

sf::Color colorFromPlayerId(quint32 playerId); // détermine la couleur d'un joueur

