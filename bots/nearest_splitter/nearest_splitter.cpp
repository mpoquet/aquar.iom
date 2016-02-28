#include <iostream>
#include <stdio.h>
#include <ainetlib16.hpp>
#include <math.h>
#include <map>

using namespace std;
using namespace ainet16;

double distance(const Position & p1, const Position & p2)
{
    double dx = p1.x - p2.x;
    double dy = p1.y - p2.y;
    return sqrt(dx*dx+dy*dy);
}

struct CellData
{
    TurnPlayerCell pcell;
    bool can_split = false;
    bool acted_this_turn = false;
};

int nearest_ncell(const Position & pos, const std::vector<NeutralCell> ncells)
{
    int imin = -1;
    float min_dist = INFINITY;

    for (unsigned int i = 0; i < ncells.size(); ++i)
    {
        const NeutralCell & ncell = ncells[i];

        if (ncell.remaining_turns_before_apparition == 0)
        {
            double dist = distance(pos, ncells[i].position);
            if (dist < min_dist)
            {
                min_dist = dist;
                imin = i;
            }
        }
    }

    return imin;
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

        while(session.is_logged())
        {
            Actions actions;

            printf("Waiting for next turn...\n");
            session.wait_for_next_turn();

            Turn turn = session.turn();

            printf("\nNew turn!\n");

            vector<NeutralCell> ncells = session.all_neutral_cells();

            map<int, CellData> my_cells;

            for (const TurnPlayerCell & cell : turn.pcells)
            {
                if (cell.player_id == session.player_id())
                {
                    CellData data;
                    data.pcell = cell;

                    if (cell.mass > 50)
                        data.can_split = true;

                    my_cells[cell.pcell_id] = data;
                }
            }

            // If all our cells are dead, let's surrender
            if (my_cells.size() == 0)
            {
                printf("Surrender\n");
                actions.add_surrender_action();
            }
            else
            {
                // If the maximum number of cells is not reached, let's split!
                int max_split_to_do = max(my_cells.size()/2, welcome.parameters.max_cells_per_player - my_cells.size());
                int nb_split_done = 0;

                for (auto mit : my_cells)
                {
                    CellData & data = mit.second;
                    if ((nb_split_done < max_split_to_do) && data.can_split)
                    {
                        int pcell_id = data.pcell.pcell_id;
                        float target_x = 0;
                        float target_y = 0;
                        float mass = data.pcell.mass / 2;
                        printf("Split action: (pcell_id=%d, pos=(%g,%g), mass=%g)\n",
                               pcell_id, target_x, target_y, mass);
                        actions.add_divide_action(pcell_id, target_x, target_y, mass);
                        ++ nb_split_done;
                    }
                    else if (ncells.size() > 0)
                    {
                        // nearest
                        int ncell_idx = nearest_ncell(data.pcell.position, ncells);

                        if (ncell_idx != -1)
                        {
                            NeutralCell ncell = ncells[ncell_idx];

                            int pcell_id = data.pcell.pcell_id;
                            float target_x = ncell.position.x;
                            float target_y = ncell.position.y;
                            printf("Move action: (pcell_id=%d, pos=(%g,%g))\n",
                                   pcell_id, target_x, target_y);
                            actions.add_move_action(data.pcell.pcell_id, ncell.position.x, ncell.position.y);

                            ncells.erase(ncells.begin() + ncell_idx);
                        }
                        else
                            printf("Bug !\n");
                    }
                    else
                    {
                        printf("Nothing to do...\n");
                    }
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
