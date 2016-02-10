#ifndef CELLGAME_HPP
#define CELLGAME_HPP

#include <QtGlobal>
#include <QTimer>

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

        float mass_absorption; // If cell A of mass mA eats cell B of mass mB and they belong to different players, mA is incremented by mass_absorption * mB
        float minimum_mass_ratio_to_absorb; // Cell A can eat cell B if and only if mA > minimum_mass_ratio_to_absorb * mB
        float minimum_pcell_mass; // The minimum allowed mass for a player cell

        float radius_factor; // cell_radius = radius_factor * cell_mass
        quint32 max_cells_per_player; // Each player cannot have more cells than this value
        float mass_loss_per_frame; // Every cell loses mass_loss_per_frame * cell_mass mass at each frame

        float base_cell_speed; // max_cell_speed = base_cell_speed - speed_loss_factor * cell_mass
        float speed_loss_factor;

        quint32 nb_initial_neutral_cells_x;
        quint32 nb_initial_neutral_cells_y;
        quint32 nb_initial_neutral_cells; // TODO; compute it from the two above parameters product
        float neutral_cells_mass;
        quint32 neutral_cells_repop_time; // An eaten cell takes neutral_cells_repop_time frames to reappear

        quint32 max_viruses; // The number of viruses cannot exceed this value
        float virus_mass;
        float virus_creation_mass_loss; // To create a virus, the cell loses virus_creation_mass_loss * cell_mass + virus_mass units of mass
        quint32 virus_max_split;

        quint32 nb_starting_cells_per_player;
        float player_cells_starting_mass;
        QVector<Position> players_starting_positions;

        quint32 nb_starting_viruses;
        QVector<Position> viruses_starting_positions;

    public:
        float compute_radius_from_mass(float mass) const;
        float compute_max_speed_from_mass(float mass) const;
    };

    struct PlayerCell
    {
        // Primary attributes
        quint32 id;
        quint32 player_id;
        Position position;
        float mass;

        // Secondary attributes (computed thanks to updateMass and its sisters)
        Position top_left;
        Position bottom_right;

        float radius;
        float radius_squared;
        float max_speed;

        quint32 remaining_isolated_turns;

        // Attributes related to the quadtree
        QuadTreeNode * responsible_node;
        QuadTreeNode * responsible_node_bbox;

    public:
        void updateMass(float new_mass, const GameParameters & parameters);
        void addMass(float mass_increment, const GameParameters & parameters);
        void removeMass(float mass_decrement, const GameParameters & parameters);

        void updateBBox(const GameParameters &parameters);
        void updateQuadtreeNodes();

        float squared_distance_to(const PlayerCell * oth_pcell) const;
        float squared_distance_to(const NeutralCell * ncell) const;
        float squared_distance_to(const Virus * virus) const;
        float squared_distance_to(const Position & pos) const;
    };

    struct NeutralCell
    {
        quint32 id;
        float mass;
        Position position;
        bool is_initial; // There are two types of neutral cells: the initial ones and those obtained by a player surrender action
        quint32 remaining_turns_before_apparition; // If an initial cell has been eaten, it disappears instantly then reappears at the same position after a certain number of turns
        QuadTreeNode * responsible_node;

        float radius;

        bool is_alive() const;
    };

    struct Virus
    {
        quint32 id;
        Position position;
        unsigned int turn_of_birth;
        QuadTreeNode * responsible_node;
    };

    struct Player
    {
        quint32 id;
        quint64 score;
        quint32 nb_cells;
        float mass;
        bool moved_this_turn;
    };


    struct MoveAction
    {
        quint32 cell_id;
        Position desired_destination;
    };

    struct DivideAction
    {
        quint32 cell_id;
        Position desired_new_cell_destination;
        float new_cell_mass;
    };

    struct CreateVirusAction
    {
        quint32 cell_id;
        Position desired_virus_destination;
    };

    struct SurrenderAction
    {
        quint32 player_id;
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
    void onPlayerMove(Client * client, int turn, QByteArray & data) override;
    void onVisuAck(Client * client, int turn, QByteArray & data) override;

    void onStart() override;
    void onPause() override;
    void onResume() override;
    void onStop() override;
    void onTurnTimerChanged(quint32 ms) override;

private slots:
    void onTurnEnd(); // Called on turn end

private:
    void compute_cell_divisions();
    void compute_virus_creations();
    void compute_cell_moves();
    void compute_player_surrenders();
    void compute_mass_loss();
    void update_pcells_remaining_isolated_turns();
    void update_dead_neutral_cells();

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

    bool is_there_opponent_pcell_in_neighbourhood(const Position & position,
                                                  float radius,
                                                  quint32 player_id);

    void make_player_repop(Player * player);
    void make_player_pop(Player * player);

    quint32 next_cell_id();
    int compress_cell_ids();

    Position compute_barycenter(const Position & a, float cA, const Position & b, float cB);

    void send_turn_to_everyone();

private:
    QMap<int, PlayerCell *> _player_cells;

    QMap<int, NeutralCell *> _initial_neutral_cells;
    QMap<int, NeutralCell *> _alive_neutral_cells;
    QList<NeutralCell *> _dead_initial_neutral_cells;

    QMap<int, Virus *> _viruses;
    QMap<int, Player*> _players;

    quint32 _next_cell_id = 0;
    int _current_turn = 0;

    bool _parameters_loaded = false;

    QTimer _timer;

    QVector<MoveAction*> _move_actions;
    QVector<DivideAction*> _divide_actions;
    QVector<CreateVirusAction*> _create_virus_actions;
    QVector<SurrenderAction*> _surrender_actions;

    GameParameters _parameters;

    QuadTreeNode * _tree_root;
};

#endif // CELLGAME_HPP
