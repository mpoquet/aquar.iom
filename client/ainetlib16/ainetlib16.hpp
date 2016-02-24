#include <string>
#include <vector>
#include <cstdint>

#include <SFML/Network.hpp>

namespace ainet16
{
    enum MetaProtocolStamp
    {
        LOGIN_PLAYER,
        LOGIN_VISU,
        LOGIN_ACK,
        LOGOUT,
        KICK,
        WELCOME,
        GAME_STARTS,
        GAME_ENDS,
        TURN,
        TURN_ACK
    };

    class Exception
    {
    public:
        Exception(const std::string & what);
        std::string what() const;

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

    struct Position
    {
        int x;
        int y;
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
        friend class Session;
    public:
        Actions();
        ~Actions();

        void clear();
        void add_move_action(int pcell_id, float position_x, float position_y);
        void add_divide_action(int pcell_id, float position_x, float position_y, float mass);
        void add_create_virus_action(int pcell_id, float position_x, float position_y);
        void add_surrender_action();

    private:
        std::vector<MoveAction> _move_actions;
        std::vector<DivideAction> _divide_actions;
        std::vector<CreateVirusAction> _create_virus_actions;
        bool _will_surrender;
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
        float radius_factor;
        int max_cells_per_player;
        float mass_loss_per_frame;
        float base_cell_speed;
        float speed_loss_factor;
        float virus_mass;
        float virus_creation_mass_loss;
        float virus_max_split;
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

        void wait_for_welcome() throw(Exception);
        void send_actions(const Actions & actions) throw(Exception);
        void wait_for_next_turn() throw(Exception);

        Welcome welcome() const;
        Turn turn() const;
        std::vector<NeutralCell> all_neutral_cells() const;

        bool is_connected() const;
        bool is_logged() const;
        bool is_player() const;

    private:
        sf::Uint8 read_uint8() throw(Exception);
        sf::Uint32 read_uint32() throw(Exception);
        sf::Uint64 read_uint64() throw(Exception);
        float read_float() throw(Exception);
        std::string read_string() throw(Exception);
        Position read_position() throw(Exception);
        bool read_bool() throw(Exception);

        void send_packet(sf::Packet & packet) throw(Exception);

    private:
        sf::TcpSocket _socket;
        bool _is_connected;
        bool _is_logged;
        bool _is_player;
        Welcome _welcome;
        Turn _turn;
        unsigned int _last_received_turn;
    };

};
