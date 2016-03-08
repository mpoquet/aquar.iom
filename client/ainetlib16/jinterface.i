%module jainl16
%{
#include "ainetlib16.hpp"
%}

%include "stl.i"
%include "std_string.i"
%include "std_vector.i"
%include "stdint.i"
%include "exception.i"

%rename(Virus) ainet16::TurnVirus;
%rename(PlayerCell) ainet16::TurnPlayerCell;
%rename(Player) ainet16::TurnPlayer;

%template(GameEndsPlayerVector) std::vector<ainet16::GameEndsPlayer>;
%template(PositionVector) std::vector<ainet16::Position>;
%template(VirusVector) std::vector<ainet16::TurnVirus>;
%template(PlayerCellVector) std::vector<ainet16::TurnPlayerCell>;
%template(PlayerVector) std::vector<ainet16::TurnPlayer>;
%template(NeutralCellVector) std::vector<ainet16::NeutralCell>;

// Allow C++ exceptions to be handled in Java
%typemap(throws, throws="java.lang.RuntimeException") ainet16::AINetException {
  jclass excep = jenv->FindClass("java/lang/RuntimeException");
  if (excep)
    jenv->ThrowNew(excep, $1.what());
  return $null;
}

// Force the AINetException Java class to extend java.lang.Exception
%typemap(javabase) ainet16::AINetException "java.lang.Exception";

// Override getMessage()
%typemap(javacode) ainet16::AINetException %{
  public String getMessage() {
    return what();
  }
%}

namespace ainet16
{
    struct GameEndsPlayer
    {
        int player_id;
        std::string name;
        uint64_t score;
    };

    class AINetException
    {
    public:
        AINetException(const std::string & what);
        virtual std::string what() const;
    };

    class DisconnectedException : public AINetException
    {
    public:
        DisconnectedException(const std::string & what = "Disconnected");
        std::string what() const;
    };

    class KickException : public AINetException
    {
    public:
        KickException(const std::string & what = "Kicked");
        std::string what() const;
    };

    class SocketErrorException : public AINetException
    {
    public:
        SocketErrorException(const std::string & what = "Socket error");
        std::string what() const;
    };

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
        std::string name;
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
        bool is_alive;
    };


    class Session
    {
    public:
        Session();
        ~Session();

        void connect(std::string address, int port) throw(AINetException);
        void login_player(std::string name) throw(AINetException);
        void login_visu(std::string name) throw(AINetException);

        Welcome wait_for_welcome() throw(AINetException);
        int wait_for_game_starts() throw(AINetException);
        void wait_for_next_turn() throw(AINetException);
        void send_actions(const Actions & actions) throw(AINetException);

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
