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

    QVector<InitialNeutralCellWelcome> initial_ncells_w;
        InitialNeutralCellWelcome p1;
        p1.position.x = 0;
        p1.position.y = 0;
        initial_ncells_w.append(p1);

        InitialNeutralCellWelcome p2;
        p2.position.x = 300;
        p2.position.y = 300;
        initial_ncells_w.append(p2);

        InitialNeutralCellWelcome p3;
        p3.position.x = 150;
        p3.position.y = 600;
        initial_ncells_w.append(p3);

    Welcome welcome = {parameters, initial_ncells_w};
    jeu.onWelcomeReceived(welcome);

    Player joueur0 = {0, 0, 0, 1};
    Player joueur1 = {1, 0, 0, 20};
    Player joueur2 = {2, 0, 0, 20};
    Player joueur3 = {3, 0, 0, 5};

    QVector<Player> players;
        players.push_back(joueur0);
        players.push_back(joueur1);
        players.push_back(joueur2);
        players.push_back(joueur3);

    Position pos = {13, 31};
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

    QVector<Virus> viruses;
        viruses.push_back(vir);

    QVector<NonInitialNeutralCell> non_initial_ncells;
        non_initial_ncells.push_back(nonInitialeNeutre);

    QVector<PlayerCell> pcells;
        pcells.push_back(joueuse0);
        pcells.push_back(joueuse1);
        pcells.push_back(joueuse2);

    QVector<InitialNeutralCell> initial_ncells;
        InitialNeutralCell cell1 = {1};
        initial_ncells.push_back(cell1);
    Turn tour = {initial_ncells, non_initial_ncells, viruses, pcells, players};

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
                switch(event.key.code) {

                case sf::Keyboard::I:
                    // todo : inverser la couleur de fond de la fenêtre et celle des contours (cellules + cadre)
                    jeu.inverseCouleurs();
                    break;

                case sf::Keyboard::T:
                    // test de la fonction onTurnReceived
                    tour.viruses[0].position.y += 100;
                    tour.pcells[0].position.x += 100;
                    tour.pcells[2].position.y -= 20;
                    tour.initial_ncells[0].remaining_turns_before_apparition = 0;
                    tour.non_initial_ncells[0].mass = 50;
                    jeu.onTurnReceived(tour);
                    tour.players[2].score = 21;
                    break;

                case sf::Keyboard::Add:
                    jeu.zoom();
                    break;

                case sf::Keyboard::Subtract:
                    jeu.dezoom();
                    break;

                case sf::Keyboard::Right:
                    jeu.deplaceVueDroite();
                    break;

                case sf::Keyboard::Left:
                    jeu.deplaceVueGauche();
                    break;

                case sf::Keyboard::Up:
                    jeu.deplaceVueHaut();
                    break;

                case sf::Keyboard::Down:
                    jeu.deplaceVueBas();
                    break;

                default:
                    break;
                }

            default:
                break;

            }
        }

        jeu.afficheTout();



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
