#include "visu.hpp"

#include <SFML/Graphics.hpp>
#include <iostream>


const int WINDOW_HEIGHT(768);
const int WINDOW_WIDTH(1024);

int main()
{
    Visu jeu;

    GameParameters parameters = {800, //map_width
                                 600, //map_height
                                 1, //min_nb_players
                                 0, //max_nb_players
                                 0, //mass_absorption
                                 0, //minimum_mass_ratio_to_absorb
                                 0, //minimum_pcell_mass
                                 4, //radius_factor
                                 0, //max_cells_per_player
                                 0, //mass_loss_per_frame
                                 0, //base_cell_speed
                                 0, //speed_loss_factor
                                 12, //virus_mass
                                 0, //virus_creation_mass_loss
                                 0, //virus_mass_split
                                 0, //nb_starting_cells_per_player
                                 0, // player_cells_starting_mass
                                 2, //initial_neutral_cells_mass
                                 0}; //neutral_cells_repop_time

    QVector<InitialNeutralCellWelcome> initial_ncells;
        InitialNeutralCellWelcome p1;
        p1.position.x = 0;
        p1.position.y = 0;
        initial_ncells.append(p1);

        InitialNeutralCellWelcome p2;
        p2.position.x = 300;
        p2.position.y = 300;
        initial_ncells.append(p2);

        InitialNeutralCellWelcome p3;
        p3.position.x = 150;
        p3.position.y = 600;
        initial_ncells.append(p3);

    Welcome welcome = {parameters, initial_ncells};

    jeu.onWelcomeReceived(welcome);

    Player joueur0 = {0, 0, 0, 1};
    Player joueur1 = {1, 0, 0, 10};
    Player joueur2 = {2, 0, 0, 20};
    Player joueur3 = {3, 0, 0, 5};
    jeu.addNewPlayer(joueur1);
    jeu.addNewPlayer(joueur2);
    jeu.addNewPlayer(joueur0);
    jeu.addNewPlayer(joueur3);
    std::cout << "Il y a " << jeu.nbeJoueurs() << " joueurs " << std::endl;

    Position pos = {300, 300};
    /*
    InitialNeutralCell initN = {0};
    Cellule* initialeNeutre = new Cellule(initN, parameters.initial_neutral_cells_mass, 0);
    jeu.addNewCell(initialeNeutre);*/

    pos = {13, 31};
    NonInitialNeutralCell nonInitialeNeutre = {10, 12, pos}; //id, mass, position

    pos = {78, 87};
    Virus vir{11, pos};
    Cellule* virus = new Cellule(vir, parameters.virus_mass);
    jeu.addNewCell(virus);

    pos = {130, 310};
    PlayerCell joueuse0 = {12, 0, pos, 14, 7};
    Cellule* cellule0 = new Cellule(joueuse0);
    jeu.addNewCell(cellule0);

    pos = {800, 420};
    PlayerCell joueuse1 = {13, 1, pos, 4, 7};
    Cellule* cellule1 = new Cellule(joueuse1);
    jeu.addNewCell(cellule1);

    pos = {800, 400};
    PlayerCell joueuse2 = {14, 2, pos, 7, 0};
    Cellule* cellule2 = new Cellule(joueuse2);
    jeu.addNewCell(cellule2);

    std::vector<Cellule*>::iterator i;
    for (i=jeu.allCellsByMass.begin(); i!=jeu.allCellsByMass.end(); ++i) {
        std::cout << "Cellule " << (*i)->id() << " masse " << (*i)->mass << std::endl;
    }

    while (jeu.window.isOpen()) {
        sf::Event event;

        // seules les fonctions pollEvent et waitEvent produisent des sf::Event
        // http://www.sfml-dev.org/tutorials/2.1/window-events.php
        while (jeu.window.pollEvent(event)) {
            switch(event.type) {

            // fermeture de la fenêtre
            case sf::Event::Closed:
                jeu.window.close();
                break;

            // appui sur un bouton du clavier
            case sf::Event::KeyPressed:
                if (event.key.code == sf::Keyboard::F) {
                    // todo : alterner entre plein écran ou fenêtré
                }

                else if (event.key.code == sf::Keyboard::I) {
                    // todo : inverser la couleur de fond de la fenêtre et celle des contours (cellules + cadre)
                    jeu.inverseCouleurs();

                }
                break;

            default:
                break;

            }
        }

        jeu.window.clear(sf::Color::White);

        jeu.afficheCadre();
        jeu.afficheToutesCellules();
        jeu.afficheScore();

        jeu.window.display(); // dessine tous les objets avec lesquels on a appelé draw

    }

    return 0;
}

/*sf::Texture texture;
//if (!texture.loadFromFile("/home/sophie/Images/Firefox_wallpaper.png")) {
if(!texture.loadFromFile("/home/sophie/Images/pikachu.png")) {
    std::cout << "Erreur lors du chargement de la texture\n";
}
texture.setSmooth(true);
texture.setRepeated(true);

sf::Sprite sprite;
sprite.setTexture(texture);
//sprite.setTextureRect(sf::IntRect(500, 500, 320, 32));
sprite.setColor(sf::Color::Red);
sprite.rotate(42);
sprite.scale(sf::Vector2f(1.5f, 1.f));

sf::Font font;
if (!font.loadFromFile("fonts/FLOWER.ttf")) {
    std::cout << "Erreur lors du chargement de la police\n";
}
sf::Text text;
text.setFont(font);
text.setString("Hello World!");
text.setCharacterSize(56);
text.setStyle(sf::Text::Bold | sf::Text::Underlined);
text.setColor(sf::Color::Cyan);
text.move(sf::Vector2f(500, 45));*/
