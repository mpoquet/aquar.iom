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
                                 10, //initial_neutral_cells_mass
                                 0}; //neutral_cells_repop_time

    QVector<InitialNeutralCellWelcome> initial_ncells;
        InitialNeutralCellWelcome p1;
        p1.position.x = 0;
        p1.position.y = 0;
        initial_ncells.append(p1);

    Welcome welcome = {parameters, initial_ncells};

    jeu.onWelcomeReceived(welcome);

    Player joueur0 = {0, 0, 0, 1};
    Player joueur1 = {1, 0, 0, 10};
    Player joueur2 = {2, 0, 0, 20};
    Player joueur3 = {3, 0, 0, 5};
    jeu.players.push_back(joueur1);
    jeu.players.push_back(joueur2);
    jeu.players.push_back(joueur0);
    jeu.players.push_back(joueur3);
    std::cout << "Il y a " << jeu.players.size() << " joueurs " << std::endl;

    Position pos = {300, 300};
    InitialNeutralCell initN = {0};
    // il manque l'information sur la position
    Cellule* initialeNeutre = new Cellule(initN, parameters.initial_neutral_cells_mass);

    pos = {13, 31};
    NonInitialNeutralCell nonInitialeNeutre = {4, 12, pos};

    pos = {78, 87};
    Virus vir{16, pos};
    Cellule* virus = new Cellule(vir, parameters.virus_mass);

    pos = {130, 310};
    PlayerCell joueuse0 = {2.1, 0, pos, 14, 7};
    Cellule* cellule0 = new Cellule(joueuse0, jeu.players.size());

    pos = {800, 400};
    PlayerCell joueuse1 = {3, 1, pos, 4, 7};
    Cellule* cellule1 = new Cellule(joueuse1, jeu.players.size());

    pos = {1000, 120};
    PlayerCell joueuse2 = {3, 2, pos, 7, 0};
    Cellule* cellule2 = new Cellule(joueuse2, jeu.players.size());


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

                if (event.key.code == sf::Keyboard::I) {
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

        jeu.afficheCellule(cellule0);
        jeu.afficheCellule(cellule1);
        jeu.afficheCellule(cellule2);
        jeu.afficheCellule(virus);
        jeu.afficheCellule(initialeNeutre);

        jeu.afficheScore();

        jeu.window.display(); // dessine tous les objets avec lesquels on a appelé draw

    }
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
