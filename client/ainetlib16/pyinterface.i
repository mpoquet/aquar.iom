%module pyainl16
%{
#include "ainetlib16.hpp"
%}

%include "std_string.i"
%include "std_vector.i"
%include "stdint.i"

%template(GameEndsPlayerVector) std::vector<ainet16::GameEndsPlayer>;
%template(PositionVector) std::vector<ainet16::Position>;
%template(TurnInitialNeutralCellVector) std::vector<ainet16::TurnInitialNeutralCell>;
%template(TurnNonInitialNeutralCellVector) std::vector<ainet16::TurnNonInitialNeutralCell>;
%template(TurnVirusVector) std::vector<ainet16::TurnVirus>;
%template(TurnPlayerCellVector) std::vector<ainet16::TurnPlayerCell>;
%template(TurnPlayerVector) std::vector<ainet16::TurnPlayer>;
%template(NeutralCellVector) std::vector<ainet16::NeutralCell>;

namespace ainet16
{
    struct GameEndsPlayer
    {
        int player_id;
        uint64_t score;
    };

    /*class Exception
    {
    public:
        Exception(const std::string & what);
        virtual std::string what() const;

    private:
        std::string _what;
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
                              std::vector<ainet16::GameEndsPlayer> players,
                              const std::string & what = "Game finished");

        int winner_player_id() const;
        std::vector<ainet16::GameEndsPlayer> players() const;
    };*/

    struct Position
    {
        float x;
        float y;
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
        std::vector<ainet16::Position> initial_ncells_positions;
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
        void wait_for_next_turn() throw(Exception);
        void send_actions(const Actions & actions) throw(Exception);

        Welcome welcome() const;
        int player_id() const;
        std::vector<ainet16::NeutralCell> neutral_cells() const;
        std::vector<ainet16::TurnPlayerCell> my_player_cells() const;
        std::vector<ainet16::TurnPlayerCell> ennemy_player_cells() const;
        std::vector<ainet16::TurnVirus> viruses() const;

        bool is_connected() const;
        bool is_logged() const;
        bool is_player() const;
    };
}

#if defined(SWIGPYTHON)
    %extend ainet16::Position {
        %pythoncode %{
            def __repr__(self):
                return '({}, {})'.format(self.x, self.y)
        %}
    }
#endif
