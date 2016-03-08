package bots;

import org.contest16.*;

public class Afk
{
    static
    {
        System.loadLibrary("jainl16");
    }

    public static void main(String[] args)
    {
        if (args.length != 2)
        {
            System.out.println("Parameters: ADDRESS PORT");
            return;
        }
        String address = args[0];
        int port = Integer.parseInt(args[1]);

        try
        {
            Session s = new Session();
            System.out.println("Connecting...");
            s.connect(address, port);
            System.out.println("Logging in...");
            s.login_player("jAFK");
            System.out.println("Waiting for welcome...");
            s.wait_for_welcome();
            System.out.println("Waiting for game starts...");
            s.wait_for_game_starts();

            while (s.is_logged())
            {
                System.out.println("Waiting for next turn...");
                s.wait_for_next_turn();

                Actions actions = new Actions();
                s.send_actions(actions);
            }
        }
        catch(java.lang.RuntimeException e)
        {
            e.printStackTrace();
        }
    }
}
