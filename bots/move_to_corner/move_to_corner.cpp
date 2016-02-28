#include <iostream>
#include <stdio.h>
#include <ainetlib16.hpp>
#include <math.h>

using namespace std;
using namespace ainet16;

double distance(ainet16::TurnPlayerCell my_cell, ainet16::NeutralCell neutral_cell) {
    return sqrt((my_cell.position.x-neutral_cell.position.x) * (my_cell.position.x-neutral_cell.position.x) + (my_cell.position.y-neutral_cell.position.y) * (my_cell.position.y-neutral_cell.position.y));
}

int main(int argc, char ** argv)
{
    if (argc != 3)
    {
        printf("Usage: %s HOSTNAME PORT\n", argv[0]);
        return 1;
    }

    string address = argv[1];
    string port_str = argv[2];
    int port = std::stoi(port_str);
    string name = "nearest";

    try
    {
        Session session;

        printf("Connecting to %s:%d...\n", address.c_str(), port);
        session.connect(address, port);

        printf("Logging in as a player named '%s'...\n", name.c_str());
        session.login_player(name);

        printf("Waiting for WELCOME...\n");
        session.wait_for_welcome();

        printf("Waiting for GAME_STARTS...\n");
        session.wait_for_game_starts();

        const float destx = 0;
        const float desty = 0;

        while(session.is_logged())
        {
            Actions actions;

            printf("Waiting for next turn...\n");
            session.wait_for_next_turn();

            Turn turn = session.turn();

            for (const TurnPlayerCell & cell : turn.pcells)
            {
                if (cell.player_id == session.player_id())
                {
                    printf("Adding move_action (pcell_id=%d, dest=(%g,%g))\n",
                           cell.pcell_id, destx, desty);
                    actions.add_move_action(cell.pcell_id, destx, desty);
                }
            }

            printf("Sending actions...\n");
            session.send_actions(actions);
        }
    }
    catch (const ainet16::GameFinishedException & e)
    {
        cout << "Game finished!" << endl;
        cout << "Winner = " << e.winner_player_id() << endl << endl;
        std::vector<ainet16::GameEndsPlayer> players = e.players();

        cout << "Player scores:" << endl;
        for (GameEndsPlayer player : players)
            printf("  (player_id=%d, score=%ld)\n", player.player_id, player.score);
    }
    catch (ainet16::Exception & exception)
    {
        cout << exception.what() << endl;
        return 1;
    }

    return 0;
}
