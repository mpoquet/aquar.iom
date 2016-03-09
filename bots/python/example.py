#!/usr/bin/python3
from pyainl16 import *
import argparse
from random import randint

parser = argparse.ArgumentParser(description='python bot example')
parser.add_argument('address', type=str, help='The server network address')
parser.add_argument('port', type=int, help='The server network port')
parser.add_argument('-name', type=str, default='pyExample', help='The bot name')

args = parser.parse_args()

try:
    session = Session()
    print("Connecting to {}:{}...".format(args.address, args.port))
    session.connect(args.address, args.port)

    print("Logging in as a player named '{}'...".format(args.name))
    session.login_player("pyExample")

    print("Waiting for welcome...")
    welcome = session.wait_for_welcome()

    ''' =======================================
        Informations within the WELCOME message
        ======================================= '''

    p = welcome.parameters
    print('Game parameters:')
    print("  map dimensions : (0,0) -> ({},{})".format(p.map_width, p.map_height))
    print("  map dimensions : (0,0) -> ({},{})".format(p.map_width, p.map_height))
    print("  there will be {} turns".format(p.nb_turns))
    print("  the number of players is in [{},{}]".format(p.min_nb_players, p.max_nb_players))

    ''' When player cell pA (belonging to player A) absorbs either a neutral cell n or
        another player cell pB (belonging to player B!=A), pA.mass is increased:
          - by mass_absorption * n.mass in the first case or
          - by mass_absorption * pB.mass in the second one. '''
    print("  mass_absorption : {}".format(p.mass_absorption))

    ''' The player cell pA (belonging to player A) must respect, in order to
        absorb player cell pB (belonging to player B!=A), the following constraint:
        pA.mass > pB.mass * minimum_mass_ratio_to_absorb '''
    print("  mass_absorption : {}".format(p.minimum_mass_ratio_to_absorb))

    print("  player cells' mass is in [{},{}]".format(p.minimum_pcell_mass, p.maximum_pcell_mass))

    ''' The radius of cell C is defined by C.radius = C.mass * radius_factor '''
    print("  radius_factor: {}".format(p.radius_factor))
    print("  the maximum number of cells any player can own is {}".format(p.max_cells_per_player))

    ''' At each turn, each player cell C loses a fraction of its mass:
        C.mass = C.mass - C.mass * mass_loss_per_frame '''
    print("  mass_loss_per_frame: {}".format(p.mass_loss_per_frame))

    ''' The maximum speed (in distance per turn) of every player cell C is defined by the following formula:
        C.max_speed = max(0, base_cell_speed - speed_loss_factor * C.mass) '''
    print("  base_cell_speed: {}".format(p.base_cell_speed))
    print("  speed_loss_factor: {}".format(p.speed_loss_factor))

    ''' When the player cell C creates a virus, it loses the following mass:
        C.mass = C.mass - virus_mass - C.mass * virus_creation_mass_loss '''
    print("  the mass of all viruses is {}".format(p.virus_mass))
    print("  virus_creation_mass_loss: {}".format(p.virus_creation_mass_loss))
    print("  one virus can create up to {} satellites".format(p.virus_max_split))

    print("  each player starts with {} cells of {} mass each".format(p.nb_starting_cells_per_player, p.player_cells_starting_mass))

    print("  the mass of initial neutral cells is {}".format(p.initial_neutral_cells_mass))
    print("  initial neutral cells need {} turns to reappear after being eaten".format(p.initial_neutral_cells_repop_time))

    print("  there are {} initial neutral cells".format(len(welcome.initial_ncells_positions)))
    print("  the unique id numbers of initial neutral cells are in [0,{}]".format(len(welcome.initial_ncells_positions) - 1))

    if len(welcome.initial_ncells_positions) > 3:
        print("The first 3 initial neutral cells:")
        for i in range(3):
            pos = welcome.initial_ncells_positions[i]
            print("  (id={}, mass={}, pos=({},{}))".format(i, p.initial_neutral_cells_mass, pos.x, pos.y))

    ''' ==========================
        End of the WELCOME message
        ========================== '''

    print("Waiting for GAME_STARTS...")
    session.wait_for_game_starts()

    while session.is_logged():
        actions = Actions()
        '''     .
               / \    If you use the same Actions instance on all turns,
              /   \   do NOT forget to clear the previously made actions
             /  !  \  between turns! This can be done by calling the
            /       \ Actions.clear() method.
            ‾‾‾‾‾‾‾‾‾                                                   '''

        print("Waiting for next turn...")
        session.wait_for_next_turn()

        print("\nTurn {}/{} received!".format(session.current_turn_number(), p.nb_turns))

        ''' The turn content can be accessed via different Session methods:
              - neutral_cells() returns all neutral cells: both initial and non-initial ones
              - player_cells() returns all player cells
              - my_player_cells() returns our player cells
              - ennemy_player_cells() returns player cells that are not ours
              - viruses() returns all viruses
              - players() returns all players '''

        print("My player cells:")
        for cell in session.my_player_cells():
            max_speed = max(0, p.base_cell_speed - cell.mass * p.speed_loss_factor)
            radius = cell.mass * p.radius_factor;
            print("  (id={}, mass={}, pos=({},{}), iso_turns={}, max_speed={}, radius={})".format(cell.pcell_id, cell.mass, cell.position.x, cell.position.y, cell.remaining_isolated_turns, max_speed, radius))

        print("Ennemy player cells:")
        for cell in session.ennemy_player_cells():
            max_speed = max(0, p.base_cell_speed - cell.mass * p.speed_loss_factor)
            radius = cell.mass * p.radius_factor;

            print("  (id={}, mass={}, player_id={}, pos=({},{}), iso_turns={}, max_speed={}, radius={})".format(cell.pcell_id, cell.mass, cell.player_id, cell.position.x, cell.position.y, cell.remaining_isolated_turns, max_speed, radius))

        print("Viruses:")
        for virus in session.viruses():
            radius = p.virus_mass * p.radius_factor;
            print("  (id={}, mass={}, pos=({},{}), radius={})".format(virus.id, p.virus_mass, virus.position.x, virus.position.y, radius))

        print("Non-initial neutral cells:")
        for cell in session.neutral_cells():
            if not cell.is_initial:
                radius = cell.mass * p.radius_factor;
                print("  (id={}, mass={}, pos=({},{}), radius={})".format(cell.id, cell.mass, cell.position.x, cell.position.y, radius))

        ''' Let us now try to do some actions! '''
        ''' Action 1: let us try to move cell (id=73) to position (x=42, y=4242) '''
        actions.add_move_action(73, 42, 4242)
        ''' Action 2: let us try to divide cell (id=51) to create a child cell of mass=3 towards the (0,0) corner '''
        actions.add_divide_action(51, 0, 0, 3)
        ''' Action 3: let us try to create a virus from cell (id=42) towards the (map_width,map_height) corner '''
        actions.add_create_virus_action(42, p.map_width, p.map_height)
        ''' Action 4: one time over 420, let us abandon our cells to get new ones the next turn '''
        if randint(1,420) == 42:
            actions.add_surrender_action()

        print("Sending actions...")
        session.send_actions(actions)

except AINetException as e:
    print("Exception received: " + e.what())
