#!/usr/bin/python3
from pyainl16 import *
import argparse

parser = argparse.ArgumentParser(description='AFK python bot example')
parser.add_argument('address', type=str, help='The server network address')
parser.add_argument('port', type=int, help='The server network port')

args = parser.parse_args()

try:
    session = Session()
    print("Connecting...")
    session.connect(args.address, args.port)
    print("Logging in...")
    session.login_player("pyAFK")
    print("Waiting for welcome...")
    welcome = session.wait_for_welcome()
    print("Waiting for game_starts...")
    session.wait_for_game_starts();

    while session.is_logged():
        print("Waiting for next turn...")
        session.wait_for_next_turn()

        actions = Actions()
        session.send_actions(actions)

except AINetException as e:
    print("Exception received: " + e.what())
