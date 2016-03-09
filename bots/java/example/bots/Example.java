package bots;

import java.util.concurrent.ThreadLocalRandom;
import java.util.Locale;

import org.contest16.*;

public class Example
{
    static
    {
        System.loadLibrary("jainl16");
    }

    public static void main(String[] args)
    {
        Locale.setDefault(new Locale("en", "US"));

        if (args.length != 2)
        {
            System.out.println("Parameters: ADDRESS PORT");
            return;
        }
        String address = args[0];
        int port = Integer.parseInt(args[1]);
        String name = "jExample";

        try
        {
            Session s = new Session();
            System.out.format("Connecting to %s:%d...\n", address, port);
            s.connect(address, port);

            System.out.format("Logging in as a player named '%s'...\n", name);
            s.login_player(name);

            System.out.println("Waiting for welcome...");
            Welcome welcome = s.wait_for_welcome();

            /* =======================================
             * Informations within the WELCOME message
             * ======================================= */

            GameParameters p = welcome.getParameters();
            System.out.format("Game parameters:\n");
            System.out.format("  map dimensions : (0,0) -> (%g,%g)\n", p.getMap_width(), p.getMap_height());
            System.out.format("  there will be %d turns\n", p.getNb_turns());
            System.out.format("  the number of players is in [%d,%d]\n", p.getMin_nb_players(), p.getMax_nb_players());

            /* When player cell pA (belonging to player A) absorbs either a neutral cell n or
               another player cell pB (belonging to player B!=A), pA.mass is increased:
                 - by mass_absorption * n.mass in the first case or
                 - by mass_absorption * pB.mass in the second one. */
            System.out.format("  mass_absorption : %g\n", p.getMass_absorption());

            /* The player cell pA (belonging to player A) must respect, in order to
               absorb player cell pB (belonging to player B!=A), the following constraint:
               pA.mass > pB.mass * minimum_mass_ratio_to_absorb */
            System.out.format("  mass_absorption : %g\n", p.getMinimum_mass_ratio_to_absorb());

            System.out.format("  player cells' mass is in [%g,%g]\n", p.getMinimum_pcell_mass(), p.getMaximum_pcell_mass());

            /* The radius of cell C is defined by C.radius = C.mass * radius_factor */
            System.out.format("  radius_factor: %g\n", p.getRadius_factor());
            System.out.format("  the maximum number of cells any player can own is %d\n", p.getMax_cells_per_player());

            /* At each turn, each player cell C loses a fraction of its mass:
               C.mass = C.mass - C.mass * mass_loss_per_frame */
            System.out.format("  mass_loss_per_frame: %g\n", p.getMass_loss_per_frame());

            /* The maximum speed (in distance per turn) of every player cell C is defined by the following formula:
               C.max_speed = max(0, base_cell_speed - speed_loss_factor * C.mass) */
            System.out.format("  base_cell_speed: %g\n", p.getBase_cell_speed());
            System.out.format("  speed_loss_factor: %g\n", p.getSpeed_loss_factor());

            /* When the player cell C creates a virus, it loses the following mass:
               C.mass = C.mass - virus_mass - C.mass * virus_creation_mass_loss */
            System.out.format("  the mass of all viruses is %g\n", p.getVirus_mass());
            System.out.format("  virus_creation_mass_loss: %g\n", p.getVirus_creation_mass_loss());
            System.out.format("  one virus can create up to %d satellites\n", p.getVirus_max_split());

            System.out.format("  each player starts with %d cells of %g mass each\n", p.getNb_starting_cells_per_player(), p.getPlayer_cells_starting_mass());

            System.out.format("  the mass of initial neutral cells is %g\n", p.getInitial_neutral_cells_mass());
            System.out.format("  initial neutral cells need %d turns to reappear after being eaten\n", p.getInitial_neutral_cells_repop_time());

            System.out.format("  there are %d initial neutral cells \n", welcome.getInitial_ncells_positions().size());
            System.out.format("  the unique id numbers of initial neutral cells are in [0,%d]\n", welcome.getInitial_ncells_positions().size() - 1);

            if (welcome.getInitial_ncells_positions().size() > 3)
            {
                System.out.format("The first 3 initial neutral cells:\n");
                for (int i = 0; i < 3; ++i)
                {
                    Position pos = welcome.getInitial_ncells_positions().get(i);
                    System.out.format("  (id=%d, mass=%g, pos=(%g,%g))\n",
                           i, p.getInitial_neutral_cells_mass(), pos.getX(), pos.getY());
                }
            }

            /* ==========================
             * End of the WELCOME message
             * ========================== */

            System.out.println("Waiting for game starts...");
            s.wait_for_game_starts();

            while (s.is_logged())
            {
                Actions actions = new Actions();
                /*     .
                 *    / \    If you use the same Actions instance on all turns,
                 *   /   \   do NOT forget to clear the previously made actions
                 *  /  !  \  between turns! This can be done by calling the
                 * /       \ Actions.clear() method.
                 * ‾‾‾‾‾‾‾‾‾                                                   */

                System.out.format("Waiting for next turn...\n");
                s.wait_for_next_turn();

                System.out.format("\nTurn %d/%d received!\n", s.current_turn_number(), p.getNb_turns());

                /* The turn content can be accessed via different Session methods:
                 *   - neutral_cells() returns all neutral cells: both initial and non-initial ones
                 *   - player_cells() returns all player cells
                 *   - my_player_cells() returns our player cells
                 *   - ennemy_player_cells() returns player cells that are not ours
                 *   - viruses() returns all viruses
                 *   - players() returns all players */

                System.out.format("My player cells:\n");
                PlayerCellVector my_pcells = s.my_player_cells();
                for (int i = 0; i < my_pcells.size(); ++i)
                {
                    PlayerCell cell = my_pcells.get(i);
                    float max_speed = Math.max(0.f, p.getBase_cell_speed() - cell.getMass() * p.getSpeed_loss_factor());
                    float radius = cell.getMass() * p.getRadius_factor();

                    System.out.format("  (id=%d, mass=%g, pos=(%g,%g), iso_turns=%d, max_speed=%g, radius=%g)\n",
                           cell.getPcell_id(), cell.getMass(), cell.getPosition().getX(), cell.getPosition().getY(),
                           cell.getRemaining_isolated_turns(), max_speed, radius);
                }

                System.out.format("Ennemy player cells:\n");
                PlayerCellVector ennemy_pcells = s.ennemy_player_cells();
                for (int i = 0; i < ennemy_pcells.size(); ++i)
                {
                    PlayerCell cell = ennemy_pcells.get(i);
                    float max_speed = Math.max(0.f, p.getBase_cell_speed() - cell.getMass() * p.getSpeed_loss_factor());
                    float radius = cell.getMass() * p.getRadius_factor();

                    System.out.format("  (id=%d, mass=%g, player_id=%d, pos=(%g,%g), iso_turns=%d, max_speed=%g, radius=%g)\n",
                           cell.getPcell_id(), cell.getMass(), cell.getPlayer_id(), cell.getPosition().getX(), cell.getPosition().getY(),
                           cell.getRemaining_isolated_turns(), max_speed, radius);
                }

                System.out.format("Viruses:\n");
                VirusVector viruses = s.viruses();
                for (int i = 0; i < viruses.size(); ++i)
                {
                    Virus virus = viruses.get(i);
                    float radius = p.getVirus_mass() * p.getRadius_factor();
                    System.out.format("  (id=%d, mass=%g, pos=(%g,%g), radius=%g)\n",
                           virus.getId(), p.getVirus_mass(), virus.getPosition().getX(),
                           virus.getPosition().getY(), radius);
                }

                System.out.format("Non-initial neutral cells:\n");
                NeutralCellVector ncells = s.neutral_cells();
                for (int i = 0; i < ncells.size(); ++i)
                {
                    NeutralCell cell = ncells.get(i);
                    if (!cell.getIs_initial())
                    {
                        float radius = cell.getMass() * p.getRadius_factor();
                        System.out.format("  (id=%d, mass=%g, pos=(%g,%g), radius=%g)\n",
                               cell.getId(), cell.getMass(), cell.getPosition().getX(),
                               cell.getPosition().getY(), radius);
                    }
                }

                // Let us now try to do some actions!

                // Action 1: let us try to move cell (id=73) to position (x=42, y=4242)
                actions.add_move_action(73, 42, 4242);
                // Action 2: let us try to divide cell (id=51) to create a child cell of mass=3 towards the (0,0) corner
                actions.add_divide_action(51, 0, 0, 3);
                // Action 3: let us try to create a virus from cell (id=42) towards the (map_width,map_height) corner
                actions.add_create_virus_action(42, p.getMap_width(), p.getMap_height());
                // Action 4: one time over 420, let us abandon our cells to get new ones the next turn
                if (ThreadLocalRandom.current().nextInt(1, 420) == 42)
                {
                    actions.add_surrender_action();
                }

                System.out.format("Sending actions...\n");
                s.send_actions(actions);
            }
        }
        catch(java.lang.RuntimeException e)
        {
            e.printStackTrace();
        }
    }
}
