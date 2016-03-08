#!/usr/bin/python3
from pyainl16 import *
import argparse
import math

def distance(p1, p2):
    dx = p1.x - p2.x
    dy = p1.y - p2.y
    return math.sqrt(dx*dx+dy*dy)

def main():
    parser = argparse.ArgumentParser(description='Nearest Virus Builder python bot example')
    parser.add_argument('address', type=str, help='The server network address')
    parser.add_argument('port', type=int, help='The server network port')

    args = parser.parse_args()

    try:
        session = Session()
        print("Connecting...")
        session.connect(args.address, args.port)
        print("Logging in...")
        session.login_player("pyNearVBuild")
        print("Waiting for welcome...")
        welcome = session.wait_for_welcome()
        print("Waiting for game_starts...")
        session.wait_for_game_starts();
        param = welcome.parameters

        while session.is_logged():
            print("Waiting for next turn...")
            session.wait_for_next_turn()

            actions = Actions()
            alive_ncells = [cell for cell in session.neutral_cells() if cell.is_alive]

            for pcell in session.my_player_cells():
                mass_to_lose = param.virus_creation_mass_loss * pcell.mass + param.virus_mass
                if pcell.mass - mass_to_lose > param.minimum_pcell_mass:
                    pid = pcell.pcell_id
                    x = pcell.position.x
                    y = pcell.position.y
                    print("Trying to create a virus (pcell_id={}, x={}, y={})".format(pid, x, y))
                    actions.add_create_virus_action(pid, x, y)
                elif alive_ncells:
                    ncells = [(distance(ncell.position, pcell.position), ncell.id, ncell.position.x, ncell.position.y) for ncell in alive_ncells]
                    ncells.sort()

                    target = ncells[0]
                    pid = pcell.pcell_id
                    x = target[2]
                    y = target[3]
                    print("Trying to move a cell (pcell_id={}, x={}, y={})".format(pid, x, y))
                    actions.add_move_action(pid, x, y)

            session.send_actions(actions)

    except AINetException as e:
        print("Exception received: " + e.what())

if __name__ == '__main__':
    main()
