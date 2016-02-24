#include <string>
#include <vector>
#include <cstdint>

namespace ainet16
{
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
    public:
        Actions();
        ~Actions();

        void clear();
        void add_move_action(int pcell_id, float position_x, float position_y);
        void add_divide_action(int pcell_id, float position_x, float position_y, float mass);
        void add_create_virus_action(int pcell_id, float position_x, float position_y);
        void add_surrender_action();

    private:
        std::vector<MoveAction> move_actions;
        std::vector<DivideAction> divide_actions;
        std::vector<CreateVirusAction> create_virus_actions;
        bool should_surrender;
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
        int neutral_cells_repop_time;
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
        NetworkLibrary();
        ~NetworkLibrary();

        void connect(std::string address, int port);
        void login_player(std::string name);
        void login_visu(std::string name);

        void wait_for_welcome();
        void send_actions(const Actions & actions);
        void wait_for_next_turn();

        Welcome welcome() const;
        Turn turn() const;
        std::vector<NeutralCell> all_neutral_cells() const;

        bool is_connected() const;
        bool is_logged() const;
        bool is_player() const;

    private:
        bool is_connected;
        bool is_logged;
        bool is_player;
        Welcome _welcome;
        Turn _turn;
    };

};
