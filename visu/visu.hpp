#pragma once

#include <QWidget>
#include <SFML/Graphics.hpp>
#include <vector>
#include <map>
#include <iostream>
#include <tgmath.h>
#include <string>
#include <QDebug>
#include <sstream>
#include <iomanip>

#include "cell.hpp"

#include <ainetlib16.hpp>

class CompareScoresJoueurs {
public:
    bool operator()(const ainet16::TurnPlayer a, const ainet16::TurnPlayer b) {
        return a.score > b.score;
    }
};

class CompareIdJoueurs {
public:
    bool operator()(const ainet16::TurnPlayer a, const ainet16::TurnPlayer b) {
        return a.player_id < b.player_id;
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
    Visu();
    ~Visu();

    void onWelcomeReceived(const ainet16::Welcome &welcome);
    void onTurnReceived(ainet16::Turn &turn, int numeroTour);

    // Affiche une cellule dans la fenêtre
    void afficheCellule(Cellule *cellule);
    // Affiche toutes les cellules
    void afficheToutesCellules();
    // Affiche les scores des joueurs à droite de la fenêtre et la répartition des scores en bas de la fenêtre
    void afficheScore();
    // Affiche le cadre qui délimite le plateau de jeu
    void afficheCadre();
    // Affiche le tour de jeu
    void afficheTour();

    // Met à jour tout l'affichage
    void afficheTout();
    void afficheFinPartie(); // utilisé quand la partie est terminée ou que la visu a une exception

    // Fait changer la couleur de l'arrière-plan entre blanc et noir
    void inverseCouleurs();

    // Gestion des évènements
    void handleEvents();
    void onGameEnd(int winnerPlayerId, std::vector<ainet16::GameEndsPlayer> endPlayers);
    void onException();

    // Ajustement du niveau de zoom
    void zoom();
    void dezoom();

    // Déplacement de la vue
    void deplaceVueDroite();
    void deplaceVueGauche();
    void deplaceVueHaut();
    void deplaceVueBas();
    void centreVueSouris();
    void resetCarte(); // Remet la vue comme au démarrage (carte entière dans le cadre)

    // Active/désactive l'affichage des cellules neutres
    void toggleNeutralCells();

    // Ajoute une cellule dans le conteneur allCells
    void addNewCell(Cellule* cellule);
    // Supprime une cellule morte
    void removeCell(quint32 id);
    // Ajoute un joueur dans le conteneur players
    void addNewPlayer(ainet16::TurnPlayer p);

// Accesseurs
    int nbeJoueurs();
    bool enCours();

// Attributs
private:
    ainet16::GameParameters parameters;
    sf::Color background_color;
    sf::Color borders_color;
    std::vector<ainet16::TurnPlayer> players;
    int nbCellulesInitiales;
    int window_height;
    int window_width;

    sf::View vue_carte; // vue qui contient la carte
    sf::View droite; // vue qui contient la liste des joueurs
    sf::View bas; // vue qui contient le diagramme des scores
    sf::View vue_cadre;
    sf::RectangleShape cadre; // zone de la fenêtre où est affichée le plateau de jeu

    bool afficheCellulesNeutres;
    bool partieEnCours; // si la partie est terminée le comportement sera différent
    int tourCourant;
    sf::Font police;

public:
    std::map<quint32, Cellule*> allCells; // toutes les cellules, tous types confondus, par m_id croissant. Les premières sont les initiales neutres
    std::vector<Cellule*> allCellsByMass; // toutes les cellules, triées par masse décroissante. Plus tard on utilisera l'algo timsort pour améliorer les performances https://github.com/gfx/cpp-TimSort

    sf::RenderWindow window; // RenderWindow hérite de Window et contient des fonctions pour le dessin
};

// Détermine la couleur d'un joueur en fonction de son identifiant et du nombre total de joueurs grâce au système hsv.
// La saturation et la valeur sont fixes mais les teintes des joueurs sont équiréparties entre 0 et 360.
sf::Color colorFromPlayerId(quint32 playerId, int nbePlayers);

// Convertit les couleurs du système hsv vers rgb.
void hsvToRgb(double h, double s, double v, double & r, double & g, double & b);

