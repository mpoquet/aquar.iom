#include <iostream>
#include <stdio.h>
#include <ainetlib16.hpp>
#include <math.h>

using namespace std;
using namespace ainet16;

double distance(const Position & p1, const Position & p2)
{
    double dx = p1.x - p2.x;
    double dy = p1.y - p2.y;
    return sqrt(dx*dx+dy*dy);
}

enum Side
{
    TOP,
    LEFT,
    BOTTOM,
    RIGHT
};

struct Objective
{
    Side side;
    Position target;
};

Objective generate_objective(const GameParameters & parameters)
{
    Objective obj;

    int r = rand() % 4;
    switch(r)
    {
    case 0: obj.side = TOP; break;
    case 1: obj.side = LEFT; break;
    case 2: obj.side = BOTTOM; break;
    case 3: obj.side = RIGHT; break;
    }

    switch(obj.side)
    {
    case TOP:
        obj.target.x = ((float)rand() / RAND_MAX) * parameters.map_width;
        obj.target.y = 0;
        break;
    case BOTTOM:
        obj.target.x = ((float)rand() / RAND_MAX) * parameters.map_width;
        obj.target.y = parameters.map_height;
        break;
    case LEFT:
        obj.target.x = 0;
        obj.target.y = ((float)rand() / RAND_MAX) * parameters.map_height;
        break;
    case RIGHT:
        obj.target.x = parameters.map_width;
        obj.target.y = ((float)rand() / RAND_MAX) * parameters.map_height;
        break;
    }

    printf("New objective : (%g,%g)\n", obj.target.x, obj.target.y);

    return obj;
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
        Welcome welcome = session.welcome();

        printf("Waiting for GAME_STARTS...\n");
        session.wait_for_game_starts();

        Objective obj = generate_objective(welcome.parameters);
        const float epsilon = 1;

        while(session.is_logged())
        {
            Actions actions;

            printf("Waiting for next turn...\n");
            session.wait_for_next_turn();

            // Let's check whether all cells reached the objective
            int nb_cells_mine = 0;
            int nb_cells_mine_at_obj = 0;

            for (const TurnPlayerCell & cell : session.my_player_cells())
            {
                ++nb_cells_mine;

                if (distance(cell.position, obj.target) < epsilon)
                    ++nb_cells_mine_at_obj;
                //actions.add_move_action(cell.pcell_id, obj.target.x, obj.target.y);
            }

            // If all our cells are dead, let's surrender
            if (nb_cells_mine == 0)
            {
                actions.add_surrender_action();
            }
            else
            {
                // If all our cells reached the objective, let it be changed
                if (nb_cells_mine == nb_cells_mine_at_obj)
                    obj = generate_objective(welcome.parameters);

                for (const TurnPlayerCell & cell : session.my_player_cells())
                    actions.add_move_action(cell.pcell_id, obj.target.x, obj.target.y);
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
    catch (ainet16::AINetException & exception)
    {
        cout << exception.what() << endl;
        return 1;
    }

    return 0;
}
