#pragma once

#include <QWidget>
#include <SFML/Graphics.hpp>
#include <vector>
#include <map>
#include <iostream>
#include <tgmath.h>
#include <string>
#include <QDebug>

#include "structures.hpp"
#include "cell.hpp"

class CompareScoresJoueurs {
public:
    bool operator()(const Player a, const Player b) {
        return a.score > b.score;
    }
};

class CompareIdJoueurs {
public:
    bool operator()(const Player a, const Player b) {
        return a.id < b.id;
    }
};

class CompareMasseCellules {
public:
    bool operator()(const Cellule* a, const Cellule* b) {
        return a->mass < b->mass;
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
    void afficheCellule(Cellule *cellule);
    // Affiche toutes les cellules
    void afficheToutesCellules();
    // Affiche les scores des joueurs à droite de la fenêtre et la répartition des scores en bas de la fenêtre
    void afficheScore();
    // Affiche le cadre qui délimite le plateau de jeu
    void afficheCadre();

    // Met à jour tout l'affichage
    void afficheTout();

    // Fait changer la couleur de l'arrière-plan entre blanc et noir
    void inverseCouleurs();

    // Gestion des évènements
    void handleEvents(Turn *tour); // todo : en version finale, ne prend plus rien en paramètre. C'est juste pour tester l'affichage

    // Ajustement du niveau de zoom
    void zoom();
    void dezoom();

    // Déplacement de la vue
    void deplaceVueDroite();
    void deplaceVueGauche();
    void deplaceVueHaut();
    void deplaceVueBas();

    // Remet la vue comme au démarrage (carte entière dans le cadre)
    void resetCarte();

    // Ajoute une cellule dans le conteneur allCells
    void addNewCell(Cellule* cellule);
    // Supprime une cellule morte
    void removeCell(quint32 id);
    // Ajoute un joueur dans le conteneur players
    void addNewPlayer(Player p);


// Accesseurs
    int nbeJoueurs();

// Attributs
private:
    GameParameters parameters;
    sf::Color background_color;
    sf::Color borders_color;
    std::vector<Player> players;
    int nbCellulesInitiales;
    int window_height;
    int window_width;

    sf::View vue_carte; // vue qui contient la carte
    sf::View droite; // vue qui contient la liste des joueurs
    sf::View bas; // vue qui contient le diagramme des scores
    sf::View vue_cadre;
    sf::RectangleShape cadre; // zone de la fenêtre où est affichée le plateau de jeu


public:
    std::map<quint32, Cellule*> allCells; // toutes les cellules, tous types confondus, par m_id croissant. Les premières sont les initiales neutres
    std::vector<Cellule*> allCellsByMass; // toutes les cellules, triées par masse décroissante. Plus tard on utilisera l'algo timsort pour améliorer les performances https://github.com/gfx/cpp-TimSort

    sf::RenderWindow window; // RenderWindow hérite de Window et contient des fonctions pour le dessin
};

// Détermine la couleur d'un joueur en fonction de son identifiant et du nombre total de joueurs grâce au système hsv.
// La saturation et la valeur sont fixes mais les teintes des joueurs sont équiréparties entre 0 et 360.
sf::Color colorFromPlayerId(quint32 playerId, int nbePlayers);

// Convertit les couleurs du système hsv vers rgb.
static void hsvToRgb(double h, double s, double v, double & r, double & g, double & b);

