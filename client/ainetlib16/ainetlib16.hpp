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

    /**
     * @brief Structure envoyée par le serveur à la fin de la partie correspondont à un joueur
     */
    struct GameEndsPlayer
    {
        int player_id; //! L'identifiant du joueur
        std::string name; //! Le nom du joueur
        uint64_t score; //! Le score du joueur
    };

    /**
     * @brief Exception utilisée pour signaler qu'une erreur a eu lieu en utilisant AINL
     */
    class AINetException : public std::runtime_error
    {
    public:
        AINetException(const std::string & what);
        virtual std::string whatstr() const;

    private:
        std::string _what;
    };

    /**
     * @brief Exception envoyée lorsque la connexion avec le serveur est perdue
     */
    class DisconnectedException : public AINetException
    {
    public:
        DisconnectedException(const std::string & what = "Disconnected");
    };

    /**
     * @brief Exception envoyée lorsque le serveur nous éjecte
     */
    class KickException : public AINetException
    {
    public:
        KickException(const std::string & what = "Kicked");
    };

    /**
     * @brief Exception envoyée lorsqu'une erreur se présente sur la socket avec le serveur
     */
    class SocketErrorException : public AINetException
    {
    public:
        SocketErrorException(const std::string & what = "Socket error");
    };

    /**
     * @brief Exception envoyée lorsque la partie s'arrête
     */
    class GameFinishedException : public AINetException
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

    /**
     * @brief Contient une paire de coordonnées 2D
     */
    struct Position
    {
        float x; //! L'abscisse, comprise dans [0,map_width[
        float y; //! L'ordonnée, comprise dans [0,map_height[
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

    /**
     * @brief Permet de stocker un ensemble d'actions pour un tour
     */
    class Actions
    {
        friend class Session;
    public:
        /**
         * @brief Crée un ensemble d'actions vide
         */
        Actions();
        /**
         * @brief Détruit un ensemble d'actions
         */
        ~Actions();

        /**
         * @brief Vide l'ensemble d'actions
         */
        void clear();

        /**
         * @brief Ajoute une action de déplacement d'une cellule
         * @param pcell_id L'identifiant unique de la cellule de joueur à déplacer
         * @param position_x L'abscisse de la position où on souhaite déplacer la cellule
         * @param position_y L'ordonnée de la position où on souhaite déplacer la cellule
         */
        void add_move_action(int pcell_id, float position_x, float position_y);
        /**
         * @brief Ajoute une action de division d'une cellule
         * @param pcell_id L'identifiant unique de la cellule de joueur à déplacer
         * @param position_x L'abscisse de la position où on souhaite créer la cellule
         * @param position_y L'ordonnée de la position où on souhaite créer la cellule
         * @param mass La masse de la cellule que l'on souhaite créer
         */
        void add_divide_action(int pcell_id, float position_x, float position_y, float mass);
        /**
         * @brief Ajoute une action de création d'un virus
         * @param pcell_id L'identifiant unique de la cellule souhaitant créer un virus
         * @param position_x L'abscisse de la position où on souhaite créer le virus
         * @param position_y L'ordonnée de la position où on souhaite creér le virus
         */
        void add_create_virus_action(int pcell_id, float position_x, float position_y);
        /**
         * @brief Indique que l'on souhaite abandonner ses cellules
         */
        void add_surrender_action();

    private:
        std::vector<MoveAction> _move_actions;
        std::vector<DivideAction> _divide_actions;
        std::vector<CreateVirusAction> _create_virus_actions;
        bool _will_surrender;
    };

    /**
     * @brief Contient les paramètres d'une partie de jeu
     */
    struct GameParameters
    {
        float map_width; //! La largeur de la carte de jeu. map_width > 0. Les abscisses x sont dans [0,map_width[
        float map_height; //! La hauteur de la carte de jeu. map_height > 0. Les ordonnées y sont dans [O,map_height[
        int min_nb_players; //! Le nombre minimum de joueurs présents au tour initial
        int max_nb_players; //! Le nombre maximum de joueurs
        float mass_absorption; //! Lorsqu'une cellule cA du joueur A absorbe une cellule cB du joueur B (B != A), la masse de A est augmentée ainsi : m(cA) = m(cA) + m(cB) * mass_absorption
        float minimum_mass_ratio_to_absorb; //! Définit le ratio nécessaire entre deux cellules de joueurs différents afin que la plus grande puisse absorber la plus petite. Pour qu'une cellule cA du joueur A puisse absorber une cellule cB du joueur B (B != A), il faut que la contrainte m(cA) > m(cB) * minimum_mass_ratio_to_absorb soit respectée.
        float minimum_pcell_mass; //! La masse minimum d'une cellule de joueur
        float maximum_pcell_mass; //! La masse maximum d'une cellule de joueur
        float radius_factor; //! Définit comment le rayon du disque représentant une cellule est calculé. r(c) = m(c) * radius_factor
        int max_cells_per_player; //! Le nombre de cellules maximum qu'un joueur peut posséder
        float mass_loss_per_frame; //! Définit la perte de masse d'une cellule par tour. La perte de masse est la suivante : m(C) = m(C) - m(C) * mass_loss_per_frame
        float base_cell_speed; //! Définit la vitesse maximum d'une cellule (en distance parcourue par tour de jeu). max_speed(C) = max(0, base_cell_speed - speed_loss_factor * m(C))
        float speed_loss_factor; //! Définit la vitesse maximum d'une cellule (en distance parcourue par tour de jeu). max_speed(C) = max(0, base_cell_speed - speed_loss_factor * m(C))
        float virus_mass; //! Définit la masse des virus
        float virus_creation_mass_loss; //! Définit la masse perdue lors de la création d'un virus. m(C) = m(C) - virus_mass - m(C) * virus_creation_mass_loss
        int virus_max_split; //! Définit le nombre maximum de satellites pouvant être créés par un virus
        int nb_starting_cells_per_player; //! Définit le nombre de cellules initiales par joueur
        float player_cells_starting_mass; //! Définit la masse initiale des cellules des joueurs
        float initial_neutral_cells_mass; //! Définit la masse des cellules neutres initiales
        int initial_neutral_cells_repop_time; //! Définit le temps (en nombre de tours) nécessaires à la réapparition d'une cellule neutre initiale
        int nb_turns; //! Définit le nombre de tours de jeu
    };

    /**
     * @brief Structure envoyée par le serveur lors d'un message WELCOME.
     */
    struct Welcome
    {
        GameParameters parameters; //! Contient les paramètres de la partie
        std::vector<Position> initial_ncells_positions; //! Contient la position des cellules neutres initiales. L'indice de chaque position correspond à l'identifiant unique de la cellule en question. Les cellules neutres initiales ont des id allant de 0 à nb_initial_neutral_cells.
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

    /**
     * @brief Contient les informations sur un virus
     */
    struct TurnVirus
    {
        int id; //! L'identifiant unique du virus
        Position position; //! La position du centre du virus
    };

    /**
     * @brief Contient les informations sur une cellule d'un joueur
     */
    struct TurnPlayerCell
    {
        int pcell_id; //! L'identifiant unique de la cellule d'un joueur
        int player_id; //! L'identifiant unique du joueur possédant la cellule
        Position position; //! La position du centre de la cellule
        float mass; //! La masse de la cellule
        int remaining_isolated_turns; //! Le nombre de tours restants pendant lesquels la cellule est isolée. La cellule est isolée si et seulement si remaining_isolated_turns > 0.
    };

    /**
     * @brief Contient les informations sur un joueur
     */
    struct TurnPlayer
    {
        int player_id; //! L'identifiant unique du joueur
        std::string name; //! Le nom du joueur
        int nb_cells; //! Le nombre de cellules appartenant au joueur
        float mass; //! La masse totale des cellules appartenant au joueur
        uint64_t score; //! Le score courant du joueur
    };

    struct Turn
    {
        std::vector<TurnInitialNeutralCell> initial_ncells;
        std::vector<TurnNonInitialNeutralCell> non_initial_ncells;
        std::vector<TurnVirus> viruses;
        std::vector<TurnPlayerCell> pcells;
        std::vector<TurnPlayer> players;
    };

    /**
     * @brief Contient les informations sur une cellule neutre
     */
    struct NeutralCell
    {
        int id; //! L'identifiant unique de la cellule neutre
        Position position; //! La position du centre de la cellule neutre
        float mass; //! La masse de la cellule neutre
        bool is_initial; //! Vaut vrai si et seulement si la cellule neutre est initiale. Une cellule neutre initiale réapparaît après un certain nombre de tours après avoir été absorbée.
        int remaining_turns_before_apparition; //! Si la cellule est initiale, elle peut être en cours de réapparition. Le nombre de tours restants avant son apparition est stocké dans cette variable. La cellule peut être absorbée si et seulement si remaining_turns_before_apparition > 0.
        bool is_alive; //! Si la cellule est initiale, elle peut être en cours de réapparition. La cellule peut être absorbée si et seulement si is_alive est vrai.
    };

    /**
     * @brief Classe principale de l'API client du concours.
     */
    class Session
    {
    public:
        /**
         * @brief Construit une session, initialement non connectée au serveur
         */
        Session();
        /**
         * @brief Détruit une session
         */
        ~Session();

        /**
         * @brief Essaye de se connecter au serveur.
         * @details S'il est impossible de se connecter au serveur, une exception est lancée.
         * @param address L'adresse du serveur. Par exemple : 192.168.0.42, 127.0.0.1, ::1...
         * @param port Le port du serveur. Par exemple : 4242.
         */
        void connect(std::string address, int port) throw(AINetException);
        /**
         * @brief Essaye de s'identifier en tant que joueur au serveur.
         * @param name Le nom que l'on souhaite utiliser
         * @pre La session doit être connectée au serveur
         */
        void login_player(std::string name) throw(AINetException);
        /**
         * @brief Essaye de s'identifier en tant que visualisation au serveur.
         * @param name Le nom que l'on souhaite utiliser
         * @pre La session doit être connectée au serveur
         */
        void login_visu(std::string name) throw(AINetException);

        /**
         * @brief Attend qu'un message WELCOME nous soit envoyé par le serveur
         * @return Le message reçu par le serveur
         */
        Welcome wait_for_welcome() throw(AINetException);
        /**
         * @brief Attend qu'un message GAME_STARTS nous soit envoyé par le serveur
         * @return Le numéro unique de joueur qui nous correspond (si le client est une visualisation, 42 est renvoyé et peut être ignoré)
         */
        int wait_for_game_starts() throw(AINetException);
        /**
         * @brief Attend qu'un message TURN nous soit envoyé par le serveur
         */
        void wait_for_next_turn() throw(AINetException);
        /**
         * @brief Envoie un ensemble d'actions au serveur, ce qui permet au serveur de savoir qu'on a réagi au tour présent et qu'on est prêt à recevoir le prochain.
         * @param actions L'ensemble d'actions que l'on souhaite effectuer
         */
        void send_actions(const Actions & actions) throw(AINetException);

        /**
         * @brief Renvoie le message de bienvenue envoyé par le serveur
         * @return Le message de bienvenue
         * @pre Le message de bienvenue a déjà été reçu : la fonction wait_for_welcome a déjà été appelée avec succès
         */
        Welcome welcome() const;

        /**
         * @brief Renvoie le tour courant envoyé par le serveur
         * @return Le tour courant
         * @pre Le tour a déjà été reçu : la fonction wait_for_next_turn a été appelée avec succès
         */
        Turn turn() const;

        /**
         * @brief Renvoie le numéro du dernier tour reçu
         * @return Le numéro du dernier tour reçu
         * @pre Le tour a déjà été reçu : la fonction wait_for_next_turn a été appelée avec succès
         */
        int current_turn_number() const;

        /**
         * @brief Renvoie le numéro de joueur nous correspondant. Cette fonction renvoie 42 si on est une visualisation et ce résultat est à ignorer.
         * @return Le numéro de joueur nous correspondant
         */
        int player_id() const;

        /**
         * @brief Calcule et renvoie les cellules neutres du tour courant
         * @return Toutes les cellules neutres du tour courant
         * @pre Le message de bienvenue et le tour ont déjà été reçus : wait_for_welcome, wait_for_game_starts puis wait_for_next_turn ont été appelées avec succès
         */
        std::vector<NeutralCell> neutral_cells() const;
        /**
         * @brief Renvoie les cellules de joueurs du tour courant
         * @return Toutes les cellules de joueurs du tour courant
         * @pre Le message de bienvenue et le tour ont déjà été reçus : wait_for_welcome, wait_for_game_starts puis wait_for_next_turn ont été appelées avec succès
         */
        std::vector<TurnPlayerCell> player_cells() const;
        /**
         * @brief Calcule et renvoie les cellules de joueur nous appartenant
         * @return Les cellules qui nous appartiennent
         * @pre Le message de bienvenue et le tour ont déjà été reçus : wait_for_welcome, wait_for_game_starts puis wait_for_next_turn ont été appelées avec succès
         */
        std::vector<TurnPlayerCell> my_player_cells() const;
        /**
         * @brief Calcule et renvoie les cellules de joueur ne nous appartenant pas
         * @return Les cellules qui ne nous appartiennent pas
         * @pre Le message de bienvenue et le tour ont déjà été reçus : wait_for_welcome, wait_for_game_starts puis wait_for_next_turn ont été appelées avec succès
         */
        std::vector<TurnPlayerCell> ennemy_player_cells() const;
        /**
         * @brief Renvoie les virus
         * @return Les virus
         * @pre Le message de bienvenue et le tour ont déjà été reçus : wait_for_welcome, wait_for_game_starts puis wait_for_next_turn ont été appelées avec succès
         */
        std::vector<TurnVirus> viruses() const;
        /**
         * @brief Renvoie les joueurs
         * @return Les joueurs
         * @pre Le message de bienvenue et le tour ont déjà été reçus : wait_for_welcome, wait_for_game_starts puis wait_for_next_turn ont été appelées avec succès
         */
        std::vector<TurnPlayer> players() const;

        /**
         * @brief Permet de savoir si on est connecté au serveur ou non
         * @return Vrai si et seulement si on est connecté au serveur
         */
        bool is_connected() const;
        /**
         * @brief Permet de savoir si on est identifié au serveur ou non
         * @return Vrai si et seulement si on est identifié au serveur
         */
        bool is_logged() const;
        /**
         * @brief Permet de savoir si on est un joueur ou non
         * @return Vrai si et seulement si on est un joueur
         * @pre On doit déjà être identifié au serveur pour que le résultat ait du sens
         */
        bool is_player() const;

    private:
        sf::Uint8 read_uint8() throw(AINetException);
        sf::Uint32 read_uint32() throw(AINetException);
        sf::Uint64 read_uint64() throw(AINetException);
        float read_float() throw(AINetException);
        std::string read_string() throw(AINetException);
        Position read_position() throw(AINetException);
        bool read_bool() throw(AINetException);
        MetaProtocolStamp read_stamp() throw(AINetException);

        void send_uint8(sf::Uint8 ui8) throw(AINetException);
        void send_uint32(sf::Uint32 ui32) throw(AINetException);
        void send_uint64(sf::Uint64 ui64) throw(AINetException);
        void send_float(float f) throw(AINetException);
        void send_string(const std::string & s) throw(AINetException);
        void send_position(const Position & pos) throw(AINetException);
        void send_bool(bool b) throw(AINetException);
        void send_stamp(MetaProtocolStamp stamp) throw(AINetException);

        std::string stamp_to_string(MetaProtocolStamp stamp) const;
        void handle_game_ends() throw (AINetException);

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
