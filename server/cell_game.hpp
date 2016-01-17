#ifndef CELLGAME_HPP
#define CELLGAME_HPP

#include "game.hpp"

class CellGame : public Game
{
    struct Position
    {
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
        float neutral_cells_mass;
        unsigned int neutral_cells_repop_time; // An eaten cell takes neutral_cells_repop_time frames to reappear

        unsigned int max_viruses; // The number of viruses cannot exceed this value
        float virus_mass;
        double virus_creation_mass_loss; // To create a virus, the cell loses virus_creation_mass_loss * cell_mass + virus_mass units of mass

        unsigned int nb_starting_cells_per_player;
        float player_cells_starting_mass;
        QVector<Position> players_starting_positions;

        unsigned int nb_starting_viruses;
        QVector<Position> viruses_starting_positions;

    public:
        float compute_radius_from_mass(float mass);
    };

    class Cell
    {
    public:
        int id;
        int player_id;
        double mass;
        Position position;

        float radius;
        unsigned int remaining_isolated_turns;
        float max_speed;

        void eatCell(const Cell * eaten_cell);

        void updateMass(double new_mass, const GameParameters & parameters);
        void addMass(double mass_increment, const GameParameters & parameters);
        void removeMass(double mass_decrement, const GameParameters & parameters);

        bool containsPosition(const Position & position) const;
    };

    class NeutralCell
    {
    public:
        int id;
        float mass;
        Position position;
        bool is_initial; // There are two types of neutral cells: the initial ones and those obtained by a player surrender action
        unsigned int remaining_turns_before_apparition; // If an initial cell has been eaten, it disappears instantly then reappears at the same position after a certain number of turns

        float radius;

        bool is_alive() const;
    };

    struct Virus
    {
        int id;
        Position position;
    };

    class Player
    {
    public:
        int id;
        unsigned long long score;
        unsigned int nb_cells;
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

    int next_cell_id();

private:
    QMap<int, Cell *> _player_cells;

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
};

#endif // CELLGAME_HPP
