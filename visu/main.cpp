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

void display_loop(const SharedData & data)
{
    float fps = 24;
    float theoretical_seconds_to_sleep = 1 / fps;

    sf::Time theoretical_time_to_sleep = sf::seconds(theoretical_seconds_to_sleep);

    bool is_window_open = true;
    bool partie_en_cours = data.visu->enCours();

    sf::Clock clock;

    while (is_window_open)
    {
        data.m->lock();
        sf::Time t1 = clock.getElapsedTime();
        is_window_open = data.visu->window.isOpen();
        partie_en_cours = data.visu->enCours();

        if (is_window_open)
        {
            data.visu->handleEvents();

            if (partie_en_cours)
            {
                data.visu->afficheTout();
            }
            else
            {
                data.visu->afficheFinPartie();
            }
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
        session.login_visu("visu");

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
            int turn_number = session.current_turn_number();
            data.visu->onTurnReceived(turn, turn_number);
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
        data.m->lock();
        data.visu->onGameEnd(e.winner_player_id(), e.players());
        data.m->unlock();
    }
    catch (const ainet16::AINetException & exception)
    {
        cout << exception.what() << endl;
        // todo: tell visu about it
        data.m->lock();
        data.visu->onException();
        data.m->unlock();
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
