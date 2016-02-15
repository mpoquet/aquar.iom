#include "visu.hpp"

#include <SFML/Graphics.hpp>
#include <iostream>


// faire un évènement pour passer en mode plein écran
int main()
{

    // tests pour la création de cellules
    InitialNeutralCell initialeNeutre;
        initialeNeutre.remaining_turns_before_apparition = 8;

    NonInitialNeutralCell nonInitialeNeutre;
        nonInitialeNeutre.id = 4;
        nonInitialeNeutre.mass = 12;
        nonInitialeNeutre.position.x = 1.3;
        nonInitialeNeutre.position.y = 3.1;

    Virus vir;
        vir.id = 16;
        vir.position.x = 78;
        vir.position.y = 87;

    PlayerCell joueuse;
        joueuse.id = 2.1;
        joueuse.player_id = 1;
        joueuse.position.x = 0;
        joueuse.position.y = 0;
        joueuse.mass = 14;
        joueuse.remaining_isolated_turns = 7;

    PlayerCell joueuse2;
        joueuse2.id = 3;
        joueuse2.player_id = 4;
        joueuse2.position.x = 0;
        joueuse2.position.y = 0;
        joueuse2.mass = 4;
        joueuse2.remaining_isolated_turns = 7;

    PlayerCell joueuse3;
        joueuse3.id = 3;
        joueuse3.player_id = 4;
        joueuse3.position.x = 12;
        joueuse3.position.y = 120;
        joueuse3.mass = 7;
        joueuse3.remaining_isolated_turns = 0;

    Cellule cellule(joueuse, 8);
    cellule.print();

    Cellule cellule2(joueuse2, 8);
    Cellule cellule3(joueuse3, 8);

    Visu jeu;

    GameParameters parameters = {0, 0, 0, 0, 0, 4, 1, 0, 0, 0, 12, 0, 0, 1, 12, 0, 0};
    QVector<InitialNeutralCellWelcome> initial_ncells;
        InitialNeutralCellWelcome p1;
        p1.position.x = 0;
        p1.position.y = 0;
        initial_ncells.append(p1);

    Welcome welcome = {parameters, initial_ncells};
    jeu.onWelcomeReceived(welcome);

    Cellule virus(vir, parameters.virus_mass);

    // Il est possible de dessiner des sprites, des textes et des formes, plus des "vertex arrays" si le reste n'est pas suffisant.

    // on crée un cercle vert de 100 pixels de rayon en 128 points
    sf::CircleShape cercle(100.f, 128);
    cercle.setFillColor(sf::Color::Green);

    sf::Texture texture;
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
    text.move(sf::Vector2f(500, 45));

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
                break;

            default:
                break;

            }
        }

        jeu.window.clear(sf::Color::White);

        Cellule* cell_ptr(&cellule);
        jeu.afficheCellule(cell_ptr);

        Cellule* cell_ptr2(&cellule2);
        jeu.afficheCellule(cell_ptr2);

        Cellule* cell_ptr3(&cellule3);
        jeu.afficheCellule(cell_ptr3);

        Cellule* vir_ptr(&virus);
        jeu.afficheCellule(vir_ptr);

        //jeu.window.draw(cercle); // dessiné dans un buffer
        //jeu.window.draw(sprite);
        //jeu.window.draw(text);
        jeu.window.display(); // dessine en vrai ce qui est dans le buffer
    }


}
