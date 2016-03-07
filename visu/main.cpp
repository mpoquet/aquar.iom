#include "visu.hpp"

#include <SFML/Graphics.hpp>
#include <iostream>
#include <ainetlib16.hpp>

#include <thread>
#include <mutex>

using namespace std;

struct SharedData
{
    Visu * visu;
    mutex * m;
    string address;
    int port;
};

using namespace std;

void testDonneesSynthese(Visu &jeu) {
    // Teste la classe Visu en utilisant des données de synthèse fabriquées de toutes pièces

    /// Création des structures nécessaires pour un tour
    ainet16::GameParameters parameters = {800, //map_width
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

    std::vector<ainet16::Position> initial_ncells_w;
    ainet16::Position p1;
    p1.x = 0;
    p1.y = 0;
    initial_ncells_w.push_back(p1);

    ainet16::Position p2;
    p2.x = 300;
    p2.y = 300;
    initial_ncells_w.push_back(p2);

    ainet16::Position p3;
    p3.x = 150;
    p3.y = 600;
    initial_ncells_w.push_back(p3);

    ainet16::Welcome welcome;
    welcome.parameters = parameters;
    welcome.initial_ncells_positions = initial_ncells_w;
    jeu.onWelcomeReceived(welcome);

    ainet16::TurnPlayer joueur0 = {0, 0, 0, 1};
    ainet16::TurnPlayer joueur1 = {1, 0, 0, 20};
    ainet16::TurnPlayer joueur2 = {2, 0, 0, 20};
    ainet16::TurnPlayer joueur3 = {3, 0, 0, 5};

    std::vector<ainet16::TurnPlayer> players;
    players.push_back(joueur0);
    players.push_back(joueur1);
    players.push_back(joueur2);
    players.push_back(joueur3);

    ainet16::Position pos = {13, 31};
    ainet16::TurnNonInitialNeutralCell nonInitialeNeutre = {10, 12, pos}; //id, mass, position

    pos = {130, 310};
    ainet16::TurnVirus vir{11, pos}; //id, position
    Cellule* virus = new Cellule(vir, parameters.virus_mass);
    jeu.addNewCell(virus);

    pos = {130, 310};
    ainet16::TurnPlayerCell joueuse0 = {12, 0, pos, parameters.virus_mass, 7};//id, player id, position, mass, isolated turns
    Cellule* cellule0 = new Cellule(joueuse0, 4);
    jeu.addNewCell(cellule0);

    pos = {600, 420};
    ainet16::TurnPlayerCell joueuse1 = {13, 1, pos, 7, 7}; //id, player id, position, mass, isolated turns
    Cellule* cellule1 = new Cellule(joueuse1, 4);
    jeu.addNewCell(cellule1);

    pos = {600, 410};
    ainet16::TurnPlayerCell joueuse2 = {14, 2, pos, 5, 0}; //id, player id, position, mass, isolated turns
    Cellule* cellule2 = new Cellule(joueuse2, 4);
    jeu.addNewCell(cellule2);

    std::vector<ainet16::TurnVirus> viruses;
    viruses.push_back(vir);

    std::vector<ainet16::TurnNonInitialNeutralCell> non_initial_ncells;
    non_initial_ncells.push_back(nonInitialeNeutre);

    std::vector<ainet16::TurnPlayerCell> pcells;
    pcells.push_back(joueuse0);
    pcells.push_back(joueuse1);
    pcells.push_back(joueuse2);

    std::vector<ainet16::TurnInitialNeutralCell> initial_ncells;
    ainet16::TurnInitialNeutralCell cell1 = {1};
    initial_ncells.push_back(cell1);
    ainet16::Turn tour = {initial_ncells, non_initial_ncells, viruses, pcells, players};

    /// test de la classe visu avec les données créées
    while (jeu.window.isOpen()) {
        jeu.handleEvents();
        jeu.afficheTout();
    }
}

void display_loop(const SharedData & data)
{
    float fps = 24;
    float theoretical_seconds_to_sleep = 1 / fps;

    sf::Time theoretical_time_to_sleep = sf::seconds(theoretical_seconds_to_sleep);

    bool is_window_open = true;

    sf::Clock clock;

    while (is_window_open)
    {
        data.m->lock();
        sf::Time t1 = clock.getElapsedTime();
        is_window_open = data.visu->window.isOpen();

        if (is_window_open)
        {
            data.visu->handleEvents();
            //printf("Display\n");
            data.visu->afficheTout();
        }

        sf::Time t2 = clock.getElapsedTime();
        data.m->unlock();

        sf::Time elapsed = t2 - t1;
        float seconds_to_sleep = std::max(0.02f, (theoretical_time_to_sleep - elapsed).asSeconds());
        sf::sleep(sf::seconds(seconds_to_sleep));
    }
}

void network_loop(const SharedData & data)
{
    try
    {
        ainet16::Session session;
        session.connect(data.address, data.port);
        session.login_visu("Ta miniblouce");

        session.wait_for_welcome();
        ainet16::Welcome welcome = session.welcome();

        data.m->lock();
        data.visu->onWelcomeReceived(welcome);
        data.m->unlock();

        session.wait_for_game_starts();

        while(session.is_logged())
        {
            session.wait_for_next_turn();
            ainet16::Turn turn = session.turn();

            data.m->lock();
            //printf("Turn update\n");
            data.visu->onTurnReceived(turn);
            data.m->unlock();

            ainet16::Actions actions;
            session.send_actions(actions);
        }
    }
    catch (const ainet16::GameFinishedException & e)
    {
        cout << "Game finished!" << endl;
        cout << "Winner = " << e.winner_player_id() << endl << endl;
        std::vector<ainet16::GameEndsPlayer> players = e.players();

        cout << "Player scores:" << endl;
        for (ainet16::GameEndsPlayer player : players)
            printf("  (player_id=%d, score=%ld)\n", player.player_id, player.score);

        // todo: tell visu who won
        cout << "appel de onGameEnd\n";
        data.visu->onGameEnd(e.winner_player_id(), e.players());
    }
    catch (const ainet16::Exception & exception)
    {
        cout << exception.what() << endl;
        // todo: tell visu about it
        data.visu->onException();
    }
}


int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s IP PORT\n", argv[0]);
        return 1;
    }

    try
    {
        SharedData data;
        data.address = argv[1];
        data.port = stoi(argv[2]);

        if (data.port < 1 || data.port > 65535)
            throw invalid_argument("Invalid port value");

        data.visu = new Visu;
        data.m = new mutex;

        thread display_thread(display_loop, data);
        thread network_thread(network_loop, data);

        display_thread.join();
        network_thread.join();

        delete data.visu;
        delete data.m;
    }
    catch (const invalid_argument & e)
    {
        printf("Invalid PORT. An integer in range [1,65535] must be provided.\n");
    }


    return 0;
}
