#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include <SFML/Network.hpp>

namespace ainet16
{
    enum MetaProtocolStamp
    {
        LOGIN_PLAYER = 0,
        LOGIN_VISU = 1,
        LOGIN_ACK = 2,
        LOGOUT = 3,
        KICK = 4,
        WELCOME = 5,
        GAME_STARTS = 6,
        GAME_ENDS = 7,
        TURN = 8,
        TURN_ACK = 9
    };

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
                              std::vector<GameEndsPlayer> players,
                              const std::string & what = "Game finished");

        int winner_player_id() const;
        std::vector<GameEndsPlayer> players() const;

    private:
        int _winner_player_id;
        std::vector<GameEndsPlayer> _players;
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

    private:
        sf::Uint8 read_uint8() throw(Exception);
        sf::Uint32 read_uint32() throw(Exception);
        sf::Uint64 read_uint64() throw(Exception);
        float read_float() throw(Exception);
        std::string read_string() throw(Exception);
        Position read_position() throw(Exception);
        bool read_bool() throw(Exception);
        MetaProtocolStamp read_stamp() throw(Exception);

        void send_uint8(sf::Uint8 ui8) throw(Exception);
        void send_uint32(sf::Uint32 ui32) throw(Exception);
        void send_uint64(sf::Uint64 ui64) throw(Exception);
        void send_float(float f) throw(Exception);
        void send_string(const std::string & s) throw(Exception);
        void send_position(const Position & pos) throw(Exception);
        void send_bool(bool b) throw(Exception);
        void send_stamp(MetaProtocolStamp stamp) throw(Exception);

        std::string stamp_to_string(MetaProtocolStamp stamp) const;
        void handle_game_ends() throw (Exception);

    private:
        sf::TcpSocket _socket;
        bool _is_connected;
        bool _is_logged;
        bool _is_player;
        Welcome _welcome;
        Turn _turn;
        unsigned int _last_received_turn = 0;
        int _player_id = -1;
        bool _debug = false;
        std::vector<std::string> _stamp_to_string_vector;
    };

}
