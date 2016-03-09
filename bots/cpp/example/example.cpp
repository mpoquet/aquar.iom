#include <iostream>
#include <stdio.h>

#include <ainetlib16.hpp>

using namespace std;
using namespace ainet16;

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
    string name = "example_cpp";

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

        /* =======================================
         * Informations within the WELCOME message
         * ======================================= */

        GameParameters p = welcome.parameters;
        printf("Game parameters:\n");
        printf("  map dimensions : (0,0) -> (%g,%g)\n", p.map_width, p.map_height);
        printf("  there will be %d turns\n", p.nb_turns);
        printf("  the number of players is in [%d,%d]\n", p.min_nb_players, p.max_nb_players);

        /* When player cell pA (belonging to player A) absorbs either a neutral cell n or
           another player cell pB (belonging to player B!=A), pA.mass is increased:
             - by mass_absorption * n.mass in the first case or
             - by mass_absorption * pB.mass in the second one. */
        printf("  mass_absorption : %g\n", p.mass_absorption);

        /* The player cell pA (belonging to player A) must respect, in order to
           absorb player cell pB (belonging to player B!=A), the following constraint:
           pA.mass > pB.mass * minimum_mass_ratio_to_absorb */
        printf("  mass_absorption : %g\n", p.minimum_mass_ratio_to_absorb);

        printf("  player cells' mass is in [%g,%g]\n", p.minimum_pcell_mass, p.maximum_pcell_mass);

        /* The radius of cell C is defined by C.radius = C.mass * radius_factor */
        printf("  radius_factor: %g\n", p.radius_factor);
        printf("  the maximum number of cells any player can own is %d\n", p.max_cells_per_player);

        /* At each turn, each player cell C loses a fraction of its mass:
           C.mass = C.mass - C.mass * mass_loss_per_frame */
        printf("  mass_loss_per_frame: %g\n", p.mass_loss_per_frame);

        /* The maximum speed (in distance per turn) of every player cell C is defined by the following formula:
           C.max_speed = max(0, base_cell_speed - speed_loss_factor * C.mass) */
        printf("  base_cell_speed: %g\n", p.base_cell_speed);
        printf("  speed_loss_factor: %g\n", p.speed_loss_factor);

        /* When the player cell C creates a virus, it loses the following mass:
           C.mass = C.mass - virus_mass - C.mass * virus_creation_mass_loss */
        printf("  the mass of all viruses is %g\n", p.virus_mass);
        printf("  virus_creation_mass_loss: %g\n", p.virus_creation_mass_loss);
        printf("  one virus can create up to %d satellites\n", p.virus_max_split);

        printf("  each player starts with %d cells of %g mass each\n", p.nb_starting_cells_per_player, p.player_cells_starting_mass);

        printf("  the mass of initial neutral cells is %g\n", p.initial_neutral_cells_mass);
        printf("  initial neutral cells need %d turns to reappear after being eaten\n", p.initial_neutral_cells_repop_time);

        printf("  there are %zu initial neutral cells \n", welcome.initial_ncells_positions.size());
        printf("  the unique id numbers of initial neutral cells are in [0,%zu]\n", welcome.initial_ncells_positions.size() - 1);

        if (welcome.initial_ncells_positions.size() > 3)
        {
            printf("The first 3 initial neutral cells:\n");
            for (int i = 0; i < 3; ++i)
            {
                Position & pos = welcome.initial_ncells_positions[i];
                printf("  (id=%d, mass=%g, pos=(%g,%g))\n",
                       i, p.initial_neutral_cells_mass, pos.x, pos.y);
            }
        }

        /* ==========================
         * End of the WELCOME message
         * ========================== */

        printf("Waiting for GAME_STARTS...\n");
        session.wait_for_game_starts();

        while(session.is_logged())
        {
            Actions actions;
            /*     .
             *    / \    If you use the same Actions instance on all turns,
             *   /   \   do NOT forget to clear the previously made actions
             *  /  !  \  between turns! This can be done by calling the
             * /       \ Actions::clear() method.
             * ‾‾‾‾‾‾‾‾‾                                                   */

            printf("Waiting for next turn...\n");
            session.wait_for_next_turn();

            printf("\nTurn %d/%d received!\n", session.current_turn_number(), p.nb_turns);

            /* The turn content can be accessed via different Session methods:
             *   - neutral_cells() returns all neutral cells: both initial and non-initial ones
             *   - player_cells() returns all player cells
             *   - my_player_cells() returns our player cells
             *   - ennemy_player_cells() returns player cells that are not ours
             *   - viruses() returns all viruses
             *   - players() returns all players */

            printf("My player cells:\n");
            for (const TurnPlayerCell & cell : session.my_player_cells())
            {
                float max_speed = max(0.f, p.base_cell_speed - cell.mass * p.speed_loss_factor);
                float radius = cell.mass * p.radius_factor;

                printf("  (id=%d, mass=%g, pos=(%g,%g), iso_turns=%d, max_speed=%g, radius=%g)\n",
                       cell.pcell_id, cell.mass, cell.position.x, cell.position.y,
                       cell.remaining_isolated_turns, max_speed, radius);
            }

            printf("Ennemy player cells:\n");
            for (const TurnPlayerCell & cell : session.ennemy_player_cells())
            {
                float max_speed = max(0.f, p.base_cell_speed - cell.mass * p.speed_loss_factor);
                float radius = cell.mass * p.radius_factor;

                printf("  (id=%d, mass=%g, player_id=%d, pos=(%g,%g), iso_turns=%d, max_speed=%g, radius=%g)\n",
                       cell.pcell_id, cell.mass, cell.player_id, cell.position.x, cell.position.y,
                       cell.remaining_isolated_turns, max_speed, radius);
            }

            printf("Viruses:\n");
            for (const TurnVirus & virus : session.viruses())
            {
                float radius = p.virus_mass * p.radius_factor;
                printf("  (id=%d, mass=%g, pos=(%g,%g), radius=%g)\n",
                       virus.id, p.virus_mass, virus.position.x,
                       virus.position.y, radius);
            }

            printf("Non-initial neutral cells:\n");
            for (const NeutralCell & cell : session.neutral_cells())
            {
                if (!cell.is_initial)
                {
                    float radius = cell.mass * p.radius_factor;
                    printf("  (id=%d, mass=%g, pos=(%g,%g), radius=%g)\n",
                           cell.id, cell.mass, cell.position.x,
                           cell.position.y, radius);
                }
            }

            // Let us now try to do some actions!

            // Action 1: let us try to move cell (id=73) to position (x=42, y=4242)
            actions.add_move_action(73, 42, 4242);
            // Action 2: let us try to divide cell (id=51) to create a child cell of mass=3 towards the (0,0) corner
            actions.add_divide_action(51, 0, 0, 3);
            // Action 3: let us try to create a virus from cell (id=42) towards the (map_width,map_height) corner
            actions.add_create_virus_action(42, p.map_width, p.map_height);
            // Action 4: one time over 420, let us abandon our cells to get new ones the next turn
            if (rand() % 420 == 42)
                actions.add_surrender_action();

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
