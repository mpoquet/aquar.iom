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
        int mon_id = session.player_id();

        printf("Waiting for GAME_STARTS...\n");
        session.wait_for_game_starts();

        std::vector<NeutralCell> cellules_neutres;
        TurnPlayerCell temp_cell;
        std::map<double, NeutralCell*> distances; // trie les cellules neutres par leur distance à ma cellule
        double temp_distance;
        NeutralCell temp_destination;
        Turn tour;
        Position destination;
        Actions actions;

        while(session.is_logged())
        {
            printf("Waiting for next turn...\n");
            session.wait_for_next_turn();

            /// recevoir le tour
            tour = session.turn();
            cellules_neutres.clear();
            distances.clear();
            cellules_neutres = session.all_neutral_cells();

            /// récupérer la liste de mes cellules
            for (unsigned int i=0; i<tour.pcells.size(); ++i) {
                if (tour.pcells[i].player_id == mon_id) {
                    temp_cell = tour.pcells[i];
                    /// trier les cellules neutres par distance croissante
                    for (unsigned int j=0; j<cellules_neutres.size(); ++j) {
                        if (cellules_neutres[j].remaining_turns_before_apparition == 0) {
                            // on ne prend en compte que les cellules vivantes
                            temp_distance = distance(temp_cell, cellules_neutres[j]);
                            distances[temp_distance] = &cellules_neutres[j];
                        }
                    }
                    /// récupérer les coord de la cellule neutre la plus proche
                    temp_destination = *(distances.begin())->second;
                    destination.x = temp_destination.position.x;
                    destination.y = temp_destination.position.y;

                    /// envoyer l'action
                    actions.add_move_action(temp_cell.pcell_id, destination.x, destination.y);
                }
            }

            printf("Sending actions...\n");
            session.send_actions(actions);
        }
    }
    catch (ainet16::Exception & exception)
    {
        cout << exception.what() << endl;
        return 1;
    }

    return 0;
}
