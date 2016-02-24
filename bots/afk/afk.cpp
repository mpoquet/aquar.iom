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

    try
    {
        Session session;
        session.connect(address, port);
        session.login_player("AFK");

        session.wait_for_welcome();
        session.wait_for_game_starts();

        while(session.is_logged())
        {
            Actions actions;
            session.send_actions(actions);

            session.wait_for_next_turn();
        }
    }
    catch (ainet16::Exception & exception)
    {
        cout << exception.what() << endl;
        return 1;
    }

    return 0;
}
