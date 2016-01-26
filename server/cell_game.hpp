#ifndef CELLGAME_HPP
#define CELLGAME_HPP

#include "game.hpp"

class CellGame : public Game
{
    struct QuadTreeNode;
    struct PlayerCell;
    struct NeutralCell;
    struct Virus;

    struct Position
    {
        Position(int x = 0, int y = 0);

        float x;
        float y;
    };

    class GameParameters
    {
    public:
        // The map goes from (0,0) to (map_width, map_height)
        float map_width;
        float map_height;

        double mass_absorption; // If cell A of mass mA eats cell B of mass mB and they belong to different players, mA is incremented by mass_absorption * mB
        double minimum_mass_ratio_to_absorb; // Cell A can eat cell B if and only if mA > minimum_mass_ratio_to_absorb * mB

        double radius_factor; // cell_radius = radius_factor * cell_mass
        unsigned int max_cells_per_player; // Each player cannot have more cells than this value
        double mass_loss_per_frame; // Every cell loses mass_loss_per_frame * cell_mass mass at each frame

        double base_cell_speed; // max_cell_speed = base_cell_speed - speed_loss_factor * cell_mass
        double speed_loss_factor;

        unsigned int nb_neutral_cells_x;
        unsigned int nb_neutral_cells_y;
        unsigned int nb_neutral_cells; // TODO; compute it from the two above parameters product
        float neutral_cells_mass;
        unsigned int neutral_cells_repop_time; // An eaten cell takes neutral_cells_repop_time frames to reappear

        unsigned int max_viruses; // The number of viruses cannot exceed this value
        float virus_mass;
        double virus_creation_mass_loss; // To create a virus, the cell loses virus_creation_mass_loss * cell_mass + virus_mass units of mass
        unsigned int virus_max_split;

        unsigned int nb_starting_cells_per_player;
        float player_cells_starting_mass;
        QVector<Position> players_starting_positions;

        unsigned int nb_starting_viruses;
        QVector<Position> viruses_starting_positions;

    public:
        float compute_radius_from_mass(float mass);
    };

    struct PlayerCell
    {
        // Primary attributes
        int id;
        int player_id;
        Position position;
        double mass;

        // Secondary attributes (computed thanks to updateMass and its sisters)
        Position top_left;
        Position bottom_right;

        float radius;
        float radius_squared;
        unsigned int remaining_isolated_turns;
        float max_speed;

        // Attributes related to the quadtree
        QuadTreeNode * responsible_node;
        QuadTreeNode * responsible_node_bbox;

    public:
        void eatCell(const PlayerCell * eaten_cell);

        void updateMass(double new_mass, const GameParameters & parameters);
        void addMass(double mass_increment, const GameParameters & parameters);
        void removeMass(double mass_decrement, const GameParameters & parameters);

        bool containsPosition(const Position & position) const;

        float squared_distance_to(const PlayerCell * oth_cell) const;
        float squared_distance_to(const NeutralCell * ncell) const;
        float squared_distance_to(const Virus * virus) const;
    };

    struct NeutralCell
    {
        int id;
        float mass;
        Position position;
        bool is_initial; // There are two types of neutral cells: the initial ones and those obtained by a player surrender action
        unsigned int remaining_turns_before_apparition; // If an initial cell has been eaten, it disappears instantly then reappears at the same position after a certain number of turns
        QuadTreeNode * responsible_node;

        float radius;

        bool is_alive() const;
    };

    struct Virus
    {
        int id;
        Position position;
        QuadTreeNode * responsible_node;
    };

    class Player
    {
    public:
        int id;
        unsigned long long score;
        unsigned int nb_cells;
    };

    enum CellType
    {
        NEUTRAL_CELL,
        PLAYER_CELL,
        VIRUS
    };

    struct IdTypeCell
    {
        int id;
        CellType type;
    };

    struct MoveAction
    {
        int cell_id;
        Position desired_destination;
    };

    struct DivideAction
    {
        int cell_id;
        Position desired_new_cell_destination;
        float new_cell_mass;
    };

    struct CreateVirusAction
    {
        int cell_id;
        Position desired_virus_destination;
    };

    struct SurrenderAction
    {
        int player_id;
    };

    /** A QuadTreeNode represents a rectangular region.
     * The region is bounded by two pairs of coordinates (the top_left and bottom_right positions).
     * Any region can contain cells. Cell C is contained in node N if:
     *   - C's bounding box entirely fits in N's region
     *   - N is the minimum-size node in which C's bounding box fits entirely.
     */
    struct QuadTreeNode
    {
        QuadTreeNode(unsigned int depth, Position top_left,
                     Position bottom_right, QuadTreeNode * parent = nullptr);

        Position top_left_position;
        Position bottom_right_position;
        Position mid_position;
        unsigned int depth;

        QuadTreeNode * parent = nullptr;
        QuadTreeNode * child_top_left = nullptr;
        QuadTreeNode * child_top_right = nullptr;
        QuadTreeNode * child_bottom_left = nullptr;
        QuadTreeNode * child_bottom_right = nullptr;

        QMap<int, PlayerCell *> player_cells; // The cells whose position is inside the node are stored here
        QMap<int, PlayerCell *> player_cells_bbox; // The cells whose bounding box is inside the node are stored here

        QMap<int, NeutralCell *> alive_neutral_cells;
        QMap<int, Virus * > viruses;

        bool contains_position(const Position & position) const;
        bool contains_rectangle(const Position & top_left, const Position & bottom_right) const;
        bool is_leaf() const;
        bool is_root() const;

    public:
        QuadTreeNode * find_responsible_node(const Position & position);
        QuadTreeNode * find_responsible_node(const Position & top_left, const Position & bottom_right);

    private:
        QuadTreeNode * find_responsible_node_r(const Position & position);
        QuadTreeNode * find_responsible_node_r(const Position & top_left, const Position & bottom_right);
    };

public:
    CellGame();
    ~CellGame();

public slots:
    void onPlayerMove(Client * client, int turn, const QByteArray & data) override;
    void onVisuAck(Client * client, int turn, const QByteArray & data) override;

private slots:
    void onTurnEnd(); // Called on turn end

private:
    void compute_cell_divisions();
    void compute_virus_creations();
    void compute_cell_moves();
    void compute_player_surrenders();
    void compute_cell_positions();
    void compute_cell_collisions();

    void compute_first_collision_pass(QSet<PlayerCell *> & pcells_to_recompute,
                                      QSet<PlayerCell *> & pcells_to_delete,
                                      QSet<PlayerCell *> & pcells_to_create,
                                      QSet<NeutralCell *> & ncells_to_delete,
                                      QSet<Virus *> & viruses_to_delete,
                                      bool & did_something);

    void compute_pcells_collisions_inside_node(PlayerCell * cell,
                                               QuadTreeNode * node,
                                               QSet<PlayerCell *> & pcells_to_recompute,
                                               QSet<PlayerCell *> & pcells_to_delete,
                                               bool & did_something);

    void compute_ncells_collisions_inside_node(PlayerCell * cell,
                                               QuadTreeNode * node,
                                               QSet<PlayerCell *> & pcells_to_recompute,
                                               QSet<NeutralCell *> & ncells_to_delete,
                                               bool & did_something);

    void compute_viruses_collisions_inside_node(PlayerCell * cell,
                                                QuadTreeNode * node,
                                                QSet<PlayerCell *> & pcells_to_recompute,
                                                QSet<PlayerCell *> & pcells_to_create,
                                                QSet<Virus *> & viruses_to_delete,
                                                bool & did_something);

    bool compute_pcell_outer_collisions_inside_node(PlayerCell * cell,
                                                    QuadTreeNode * node,
                                                    QSet<PlayerCell *> & pcells_to_recompute,
                                                    bool & did_something);

    void make_player_reappear(Player * player);

    int next_cell_id();
    int compress_cell_ids();

    Position compute_barycenter(const Position & a, float cA, const Position & b, float cB);

private:
    QMap<int, PlayerCell *> _player_cells;

    QMap<int, NeutralCell *> _initial_neutral_cells;
    QMap<int, NeutralCell *> _alive_neutral_cells;
    QVector<NeutralCell *> _dead_initial_neutral_cells; // sorted by ascending remaining_turns_before_apparition

    QMap<int, Virus *> _viruses;
    QMap<int, Player*> _players;

    int _next_cell_id = 0;

    QVector<MoveAction*> _move_actions;
    QVector<DivideAction*> _divide_actions;
    QVector<CreateVirusAction*> _create_virus_actions;
    QVector<SurrenderAction*> _surrender_actions;

    GameParameters _parameters;

    QuadTreeNode * _tree_root;
};

#endif // CELLGAME_HPP
