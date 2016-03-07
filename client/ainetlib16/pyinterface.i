%module pyainl16
%{
#include "ainetlib16.hpp"
%}

// Very simple C++ example for linked list

namespace ainet16
{
    struct GameEndsPlayer
    {
        int player_id;
        uint64_t score;
    };

    class Exception
    {
    public:
        Exception(const std::string & what);
        virtual std::string what() const;
    };

    class DisconnectedException : public Exception
    {
    public:
        DisconnectedException(const std::string & what = "Disconnected");
    };

    class KickException : public Exception
    {
    public:
        KickException(const std::string & what = "Kicked");
    };

    class SocketErrorException : public Exception
    {
    public:
        SocketErrorException(const std::string & what = "Socket error");
    };

    class GameFinishedException : public Exception
    {
    public:
        GameFinishedException(int winner_player_id,
                              std::vector<GameEndsPlayer> players,
                              const std::string & what = "Game finished");

        int winner_player_id() const;
        std::vector<GameEndsPlayer> players() const;
    };

    struct Position
    {
        float x;
        float y;
    };

    struct MoveAction
    {
        int pcell_id;
        Position position;
    };

    struct DivideAction
    {
        int pcell_id;
        Position position;
        float mass;
    };

    struct CreateVirusAction
    {
        int pcell_id;
        Position position;
    };

    class Actions
    {
    public:
        Actions();
        ~Actions();

        void clear();
        void add_move_action(int pcell_id, float position_x, float position_y);
        void add_divide_action(int pcell_id, float position_x, float position_y, float mass);
        void add_create_virus_action(int pcell_id, float position_x, float position_y);
        void add_surrender_action();
    };



    struct GameParameters
    {
        float map_width;
        float map_height;
        int min_nb_players;
        int max_nb_players;
        float mass_absorption;
        float minimum_mass_ratio_to_absorb;
        float minimum_pcell_mass;
        float maximum_pcell_mass;
        float radius_factor;
        int max_cells_per_player;
        float mass_loss_per_frame;
        float base_cell_speed;
        float speed_loss_factor;
        float virus_mass;
        float virus_creation_mass_loss;
        int virus_max_split;
        int nb_starting_cells_per_player;
        float player_cells_starting_mass;
        float initial_neutral_cells_mass;
        int initial_neutral_cells_repop_time;
    };

    struct Welcome
    {
        GameParameters parameters;
        std::vector<Position> initial_ncells_positions;
    };

    struct TurnInitialNeutralCell
    {
        int remaining_turns_before_apparition;
    };

    struct TurnNonInitialNeutralCell
    {
        int ncell_id;
        float mass;
        Position position;
    };

    struct TurnVirus
    {
        int id;
        Position position;
    };

    struct TurnPlayerCell
    {
        int pcell_id;
        int player_id;
        Position position;
        float mass;
        int remaining_isolated_turns;
    };

    struct TurnPlayer
    {
        int player_id;
        int nb_cells;
        float mass;
        uint64_t score;
    };

    struct Turn
    {
        std::vector<TurnInitialNeutralCell> initial_ncells;
        std::vector<TurnNonInitialNeutralCell> non_initial_ncells;
        std::vector<TurnVirus> viruses;
        std::vector<TurnPlayerCell> pcells;
        std::vector<TurnPlayer> players;
    };

    struct NeutralCell
    {
        int id;
        Position position;
        float mass;
        bool is_initial;
        int remaining_turns_before_apparition;
    };



    class Session
    {
    public:
        Session();
        ~Session();

        void connect(std::string address, int port) throw(Exception);
        void login_player(std::string name) throw(Exception);
        void login_visu(std::string name) throw(Exception);

        Welcome wait_for_welcome() throw(Exception);
        int wait_for_game_starts() throw(Exception);
        Turn wait_for_next_turn() throw(Exception);
        void send_actions(const Actions & actions) throw(Exception);

        Welcome welcome() const;
        Turn turn() const;
        int player_id() const;
        std::vector<NeutralCell> neutral_cells() const;
        std::vector<TurnPlayerCell> my_player_cells() const;
        std::vector<TurnPlayerCell> ennemy_player_cells() const;
        std::vector<TurnVirus> viruses() const;

        bool is_connected() const;
        bool is_logged() const;
        bool is_player() const;
    };
}
