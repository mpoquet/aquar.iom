#include <ainetlib16.hpp>

using namespace std;

ainet16::AINetException::AINetException(const string & what) :
    std::runtime_error(what)
{
    _what = what;
}

string ainet16::AINetException::whatstr() const
{
    return _what;
}

ainet16::Actions::Actions()
{
    clear();
}

ainet16::Actions::~Actions()
{

}

void ainet16::Actions::clear()
{
    _move_actions.clear();
    _divide_actions.clear();
    _create_virus_actions.clear();
    _will_surrender = false;
}

void ainet16::Actions::add_move_action(int pcell_id, float position_x, float position_y)
{
    MoveAction action;
    action.pcell_id = pcell_id;
    action.position.x = position_x;
    action.position.y = position_y;

    _move_actions.push_back(action);
}

void ainet16::Actions::add_divide_action(int pcell_id, float position_x, float position_y, float mass)
{
    DivideAction action;
    action.pcell_id = pcell_id;
    action.position.x = position_x;
    action.position.y = position_y;
    action.mass = mass;

    _divide_actions.push_back(action);
}

void ainet16::Actions::add_create_virus_action(int pcell_id, float position_x, float position_y)
{
    CreateVirusAction action;
    action.pcell_id = pcell_id;
    action.position.x = position_x;
    action.position.y = position_y;

    _create_virus_actions.push_back(action);
}

void ainet16::Actions::add_surrender_action()
{
    _will_surrender = true;
}



ainet16::Session::Session()
{
    _stamp_to_string_vector = {"LOGIN_PLAYER",
    "LOGIN_VISU",
    "LOGIN_ACK",
    "LOGOUT",
    "KICK",
    "WELCOME",
    "GAME_STARTS",
    "GAME_ENDS",
    "TURN",
    "TURN_ACK"};
}

ainet16::Session::~Session()
{
    _socket.disconnect();
}

void ainet16::Session::connect(string address, int port) throw(AINetException)
{
    if (port < 0 || port > 65535)
        throw AINetException("Bad port");

    sf::Socket::Status status = _socket.connect(sf::IpAddress(address), port);

    if (status != sf::Socket::Done)
        throw AINetException("Impossible to connect on socket");

    _is_connected = true;
}

void ainet16::Session::login_player(string name) throw(AINetException)
{
    try
    {
        // Sending LOGIN_PLAYER message
        send_uint8(LOGIN_PLAYER);
        send_string(name);

        // Waiting for LOGIN_ACK
        MetaProtocolStamp stamp = read_stamp();

        if (stamp == MetaProtocolStamp::LOGIN_ACK)
        {
            _is_logged = true;
            _is_player = true;
            return;
        }
        else if (stamp == MetaProtocolStamp::KICK)
        {
            string kick_reason = read_string();
            throw KickException(kick_reason);
        }
        else
            throw AINetException("Invalid response to LOGIN_PLAYER, received_stamp=" + stamp_to_string(stamp));
    }
    catch (const SocketErrorException & e)
    {
        throw SocketErrorException(e.whatstr() + ", while attempting to login_player");
    }
    catch (const DisconnectedException & e)
    {
        throw DisconnectedException(e.whatstr() + ", while attempting to login_player");
    }
}

void ainet16::Session::login_visu(string name) throw(AINetException)
{
    try
    {
        // Sending LOGIN_VISU message
        send_uint8(LOGIN_VISU);
        send_string(name);

        // Waiting for LOGIN_ACK
        MetaProtocolStamp stamp = read_stamp();

        if (stamp == MetaProtocolStamp::LOGIN_ACK)
        {
            _is_logged = true;
            _is_player = false;
            return;
        }
        else if (stamp == MetaProtocolStamp::KICK)
        {
            string kick_reason = read_string();
            throw KickException(kick_reason);
        }
        else
            throw AINetException("Invalid response to LOGIN_VISU, received_stamp=" + stamp_to_string(stamp));
    }
    catch (const SocketErrorException & e)
    {
        throw SocketErrorException(e.whatstr() + ", while attempting to login_visu");
    }
    catch (const DisconnectedException & e)
    {
        throw DisconnectedException(e.whatstr() + ", while attempting to login_visu");
    }
}

ainet16::Welcome ainet16::Session::wait_for_welcome() throw(AINetException)
{
    try
    {
        // Metaprotocol
        MetaProtocolStamp stamp = read_stamp();

        if (stamp != MetaProtocolStamp::WELCOME)
        {
            if (stamp == MetaProtocolStamp::KICK)
            {
                string kick_reason = read_string();
                throw KickException(kick_reason);
            }
            else
                throw AINetException("Invalid stamp received while waiting for WELCOME, received_stamp=" + stamp_to_string(stamp));
        }

        sf::Uint32 gdc_size = read_uint32();
        (void) gdc_size;

        // Game-dependent protocol
        // Reading the game parameters
        _welcome.parameters.map_width = read_float();
        _welcome.parameters.map_height = read_float();
        _welcome.parameters.min_nb_players = read_uint32();
        _welcome.parameters.max_nb_players = read_uint32();
        _welcome.parameters.mass_absorption = read_float();
        _welcome.parameters.minimum_mass_ratio_to_absorb = read_float();
        _welcome.parameters.minimum_pcell_mass = read_float();
        _welcome.parameters.maximum_pcell_mass = read_float();
        _welcome.parameters.radius_factor = read_float();
        _welcome.parameters.max_cells_per_player = read_uint32();
        _welcome.parameters.mass_loss_per_frame = read_float();
        _welcome.parameters.base_cell_speed = read_float();
        _welcome.parameters.speed_loss_factor = read_float();
        _welcome.parameters.virus_mass = read_float();
        _welcome.parameters.virus_creation_mass_loss = read_float();
        _welcome.parameters.virus_max_split = read_uint32();
        _welcome.parameters.nb_starting_cells_per_player = read_uint32();
        _welcome.parameters.player_cells_starting_mass = read_float();
        _welcome.parameters.initial_neutral_cells_mass = read_float();
        _welcome.parameters.initial_neutral_cells_repop_time = read_uint32();

        // Reading the position of the initial neutral cells
        sf::Uint32 nb_initial_neutral_cells = read_uint32();
        _welcome.initial_ncells_positions.resize(nb_initial_neutral_cells);

        for (Position & position : _welcome.initial_ncells_positions)
        {
            position.x = read_float();
            position.y = read_float();
        }

        if (_debug)
        {
            printf("  map_width=%f\n", _welcome.parameters.map_width);
            printf("  map_height=%f\n", _welcome.parameters.map_height);
            printf("  min_nb_players=%d\n", _welcome.parameters.min_nb_players);
            printf("  max_nb_players=%d\n", _welcome.parameters.max_nb_players);
            printf("  mass_absorption=%f\n", _welcome.parameters.mass_absorption);
            printf("  minimum_mass_ratio_to_absorb=%f\n", _welcome.parameters.minimum_mass_ratio_to_absorb);
            printf("  minimum_pcell_mass=%f\n", _welcome.parameters.minimum_pcell_mass);
            printf("  maximum_pcell_mass=%f\n", _welcome.parameters.maximum_pcell_mass);
            printf("  radius_factor=%f\n", _welcome.parameters.radius_factor);
            printf("  max_cells_per_player=%d\n", _welcome.parameters.max_cells_per_player);
            printf("  mass_loss_per_frame=%f\n", _welcome.parameters.mass_loss_per_frame);
            printf("  base_cell_speed=%f\n", _welcome.parameters.base_cell_speed);
            printf("  speed_loss_factor=%f\n", _welcome.parameters.speed_loss_factor);
            printf("  virus_mass=%f\n", _welcome.parameters.virus_mass);
            printf("  virus_creation_mass_loss=%f\n", _welcome.parameters.virus_creation_mass_loss);
            printf("  virus_max_split=%d\n", _welcome.parameters.virus_max_split);
            printf("  nb_starting_cells_per_player=%d\n", _welcome.parameters.nb_starting_cells_per_player);
            printf("  player_cells_starting_mass=%f\n", _welcome.parameters.player_cells_starting_mass);
            printf("  initial_neutral_cells_mass=%f\n", _welcome.parameters.initial_neutral_cells_mass);
            printf("  initial_neutral_cells_repop_time=%d\n", _welcome.parameters.initial_neutral_cells_repop_time);

            printf("  nb_initial_ncells=%d\n", (int)nb_initial_neutral_cells);

            /*for (Position & position : _welcome.initial_ncells_positions)
                printf("    (%f, %f)\n", position.x, position.y);*/
        }

        return _welcome;
    }
    catch (const SocketErrorException & e)
    {
        throw SocketErrorException(e.whatstr() + ", while attempting to wait_for_welcome");
    }
    catch (const DisconnectedException & e)
    {
        throw DisconnectedException(e.whatstr() + ", while attempting to wait_for_welcome");
    }
}

int ainet16::Session::wait_for_game_starts() throw(AINetException)
{
    try
    {
        // Metaprotocol
        MetaProtocolStamp stamp = read_stamp();

        if (stamp != MetaProtocolStamp::GAME_STARTS)
        {
            if (stamp == MetaProtocolStamp::KICK)
            {
                string kick_reason = read_string();
                throw KickException(kick_reason);
            }
            else
                throw AINetException("Invalid stamp received while waiting for GAME_STARTS, received_stamp=" + stamp_to_string(stamp));
        }

        sf::Uint32 gdc_size = read_uint32();
        (void) gdc_size;

        // Game-dependent protocol
        _player_id = read_uint32();
        // Read initial neutral cells' positions
        sf::Uint32 nb_initial_ncells = read_uint32();
        if (nb_initial_ncells != _welcome.initial_ncells_positions.size())
            throw AINetException("Incoherent number of initial neutral cells received (welcome/turn)");

        _turn.initial_ncells.resize(nb_initial_ncells);
        for (TurnInitialNeutralCell & ncell : _turn.initial_ncells)
            ncell.remaining_turns_before_apparition = read_uint32();

        // Read non-initial neutral cells
        sf::Uint32 nb_non_initial_ncells = read_uint32();
        _turn.non_initial_ncells.resize(nb_non_initial_ncells);
        for (TurnNonInitialNeutralCell & ncell : _turn.non_initial_ncells)
        {
            ncell.ncell_id = read_uint32();
            ncell.mass = read_float();
            ncell.position = read_position();
        }

        // Read viruses
        sf::Uint32 nb_viruses = read_uint32();
        _turn.viruses.resize(nb_viruses);
        for (TurnVirus & virus : _turn.viruses)
        {
            virus.id = read_uint32();
            virus.position = read_position();
        }

        // Read player cells
        sf::Uint32 nb_pcells = read_uint32();
        _turn.pcells.resize(nb_pcells);
        for (TurnPlayerCell & pcell : _turn.pcells)
        {
            pcell.pcell_id = read_uint32();
            pcell.position = read_position();
            pcell.player_id = read_uint32();
            pcell.mass = read_float();
            pcell.remaining_isolated_turns = read_uint32();
        }

        // Read players
        sf::Uint32 nb_players = read_uint32();
        _turn.players.resize(nb_players);
        for (TurnPlayer & player : _turn.players)
        {
            player.player_id = read_uint32();
            player.name = read_string();
            player.nb_cells = read_uint32();
            player.mass = read_float();
            player.score = read_uint64();
        }

        if (_debug)
        {
            printf("  player_id: %d\n", _player_id);
            printf("  Turn number: %d\n", _last_received_turn);
            printf("  Initial ncells:\n");
            //for (const TurnInitialNeutralCell & ncell : _turn.initial_ncells)
            //    printf("    %d\n", ncell.remaining_turns_before_apparition);
            printf("    {not displayed}\n");

            printf("  Non-initial ncells:\n");
            for (const TurnNonInitialNeutralCell & ncell : _turn.non_initial_ncells)
                printf("    (id=%d,mass=%g,pos=(%g,%g))\n", ncell.ncell_id, ncell.mass,
                       ncell.position.x, ncell.position.y);

            printf("  Viruses:\n");
            for (const TurnVirus & virus : _turn.viruses)
                printf("    (id=%d,pos=(%g,%g))\n", virus.id, virus.position.x,
                       virus.position.y);

            printf("  Player cells:\n");
            for (const TurnPlayerCell & pcell : _turn.pcells)
                printf("    (id=%d,pid=%d,iso=%d,mass=%g,pos=(%g,%g))\n", pcell.pcell_id,
                       pcell.player_id, pcell.remaining_isolated_turns,
                       pcell.mass, pcell.position.x, pcell.position.y);

            printf("  Players:\n");
            for (const TurnPlayer & player : _turn.players)
                printf("    (id=%d,name='%s',nb_pcells=%d,mass=%g,score=%ld)\n",
                       player.player_id, player.name.c_str(), player.nb_cells,
                       player.mass, player.score);
        }
        return _player_id;

    }
    catch (const SocketErrorException & e)
    {
        throw SocketErrorException(e.whatstr() + ", while attempting to wait_for_game_starts");
    }
    catch (const DisconnectedException & e)
    {
        throw DisconnectedException(e.whatstr() + ", while attempting to wait_for_game_starts");
    }
}

void ainet16::Session::send_actions(const ainet16::Actions &actions) throw(AINetException)
{
    try
    {
        // Compute game-dependent content size
        sf::Uint32 gdc_size = 0;

        gdc_size += 4 + actions._move_actions.size() * (4 + 4*2);
        gdc_size += 4 + actions._divide_actions.size() * (4 + 4*2 + 4);
        gdc_size += 4 + actions._create_virus_actions.size() * (4 + 4*2);
        gdc_size += 1;

        // Metaprotocol header
        send_uint8(MetaProtocolStamp::TURN_ACK);
        send_uint32(gdc_size);
        send_uint32(_last_received_turn);

        // Game-dependent content
        // Send move actions
        send_uint32(actions._move_actions.size());
        for (const MoveAction & action : actions._move_actions)
        {
            send_uint32(action.pcell_id);
            send_position(action.position);
        }

        // Send divide actions
        send_uint32(actions._divide_actions.size());
        for (const DivideAction & action : actions._divide_actions)
        {
            send_uint32(action.pcell_id);
            send_position(action.position);
            send_float(action.mass);
        }

        // Send create virus actions
        send_uint32(actions._create_virus_actions.size());
        for (const CreateVirusAction & action : actions._create_virus_actions)
        {
            send_uint32(action.pcell_id);
            send_position(action.position);
        }

        // Send surrender action
        send_bool(actions._will_surrender);
    }
    catch (const SocketErrorException & e)
    {
        throw SocketErrorException(e.whatstr() + ", while attempting to send_actions");
    }
    catch (const DisconnectedException & e)
    {
        throw DisconnectedException(e.whatstr() + ", while attempting to send_actions");
    }
}

void ainet16::Session::wait_for_next_turn() throw(AINetException)
{
    try
    {
        // Metaprotocol
        MetaProtocolStamp stamp = read_stamp();

        if (stamp != MetaProtocolStamp::TURN)
        {
            if (stamp == MetaProtocolStamp::KICK)
            {
                string kick_reason = read_string();
                throw KickException(kick_reason);
            }
            else if (stamp == MetaProtocolStamp::GAME_ENDS)
            {
                handle_game_ends();
            }
            else
                throw AINetException("Invalid stamp received while waiting for TURN, received_stamp=" + stamp_to_string(stamp));
        }

        sf::Uint32 gdc_size = read_uint32();
        (void) gdc_size;

        const unsigned int last_received_turn = read_uint32();
        if (last_received_turn > _last_received_turn + 1)
            printf("Warning: %d turns have been skipped before receiving turn %d\n",
                   last_received_turn - _last_received_turn, last_received_turn);
        _last_received_turn = last_received_turn;

        // Game-dependent protocol
        // Read initial neutral cells' positions
        sf::Uint32 nb_initial_ncells = read_uint32();
        if (nb_initial_ncells != _welcome.initial_ncells_positions.size())
            throw AINetException("Incoherent number of initial neutral cells received (welcome/turn)");

        _turn.initial_ncells.resize(nb_initial_ncells);
        for (TurnInitialNeutralCell & ncell : _turn.initial_ncells)
            ncell.remaining_turns_before_apparition = read_uint32();

        // Read non-initial neutral cells
        sf::Uint32 nb_non_initial_ncells = read_uint32();
        _turn.non_initial_ncells.resize(nb_non_initial_ncells);
        for (TurnNonInitialNeutralCell & ncell : _turn.non_initial_ncells)
        {
            ncell.ncell_id = read_uint32();
            ncell.mass = read_float();
            ncell.position = read_position();
        }

        // Read viruses
        sf::Uint32 nb_viruses = read_uint32();
        _turn.viruses.resize(nb_viruses);
        for (TurnVirus & virus : _turn.viruses)
        {
            virus.id = read_uint32();
            virus.position = read_position();
        }

        // Read player cells
        sf::Uint32 nb_pcells = read_uint32();
        _turn.pcells.resize(nb_pcells);
        for (TurnPlayerCell & pcell : _turn.pcells)
        {
            pcell.pcell_id = read_uint32();
            pcell.position = read_position();
            pcell.player_id = read_uint32();
            pcell.mass = read_float();
            pcell.remaining_isolated_turns = read_uint32();
        }

        // Read players
        sf::Uint32 nb_players = read_uint32();
        _turn.players.resize(nb_players);
        for (TurnPlayer & player : _turn.players)
        {
            player.player_id = read_uint32();
            player.name = read_string();
            player.nb_cells = read_uint32();
            player.mass = read_float();
            player.score = read_uint64();
        }

        if (_debug)
        {
            printf("  Turn number: %d\n", _last_received_turn);
            printf("  Initial ncells:\n");
            //for (const TurnInitialNeutralCell & ncell : _turn.initial_ncells)
            //    printf("    %d\n", ncell.remaining_turns_before_apparition);
            printf("    {not displayed}\n");

            printf("  Non-initial ncells:\n");
            for (const TurnNonInitialNeutralCell & ncell : _turn.non_initial_ncells)
                printf("    (id=%d,mass=%g,pos=(%g,%g))\n", ncell.ncell_id, ncell.mass,
                       ncell.position.x, ncell.position.y);

            printf("  Viruses:\n");
            for (const TurnVirus & virus : _turn.viruses)
                printf("    (id=%d,pos=(%g,%g))\n", virus.id, virus.position.x,
                       virus.position.y);

            printf("  Player cells:\n");
            for (const TurnPlayerCell & pcell : _turn.pcells)
                printf("    (id=%d,pid=%d,iso=%d,mass=%g,pos=(%g,%g))\n", pcell.pcell_id,
                       pcell.player_id, pcell.remaining_isolated_turns,
                       pcell.mass, pcell.position.x, pcell.position.y);

            printf("  Players:\n");
            for (const TurnPlayer & player : _turn.players)
                printf("    (id=%d,name='%s',nb_pcells=%d,mass=%g,score=%ld)\n",
                       player.player_id, player.name.c_str(), player.nb_cells,
                       player.mass, player.score);
        }
    }
    catch (const SocketErrorException & e)
    {
        throw SocketErrorException(e.whatstr() + ", while attempting to wait_for_next_turn");
    }
    catch (const DisconnectedException & e)
    {
        throw DisconnectedException(e.whatstr() + ", while attempting to wait_for_next_turn");
    }
}

ainet16::Welcome ainet16::Session::welcome() const
{
    return _welcome;
}

ainet16::Turn ainet16::Session::turn() const
{
    return _turn;
}

int ainet16::Session::player_id() const
{
    if (_player_id == -1)
        throw AINetException("Session::player_id cannot be called before receiving the GAME_STARTS message");
    return _player_id;
}

std::vector<ainet16::NeutralCell> ainet16::Session::neutral_cells() const
{
    std::vector<ainet16::NeutralCell> res;
    res.resize(_welcome.initial_ncells_positions.size() + _turn.non_initial_ncells.size());

    int i = 0;

    // Sets initial neutral cells
    for (const Position & pos : _welcome.initial_ncells_positions)
    {
        res[i].id = i;
        res[i].position = pos;
        res[i].mass = _welcome.parameters.initial_neutral_cells_mass;
        res[i].is_initial = true;
        res[i].remaining_turns_before_apparition = _turn.initial_ncells[i].remaining_turns_before_apparition;
        res[i].is_alive = res[i].remaining_turns_before_apparition == 0;

        i++;
    }

    // Sets non-initial neutral cells
    for (const TurnNonInitialNeutralCell & ncell : _turn.non_initial_ncells)
    {
        res[i].id = ncell.ncell_id;
        res[i].position = ncell.position;
        res[i].mass = ncell.mass;
        res[i].is_initial = false;
        res[i].remaining_turns_before_apparition = 0;

        i++;
    }

    return res;
}

std::vector<ainet16::TurnPlayerCell> ainet16::Session::player_cells() const
{
    return _turn.pcells;
}

std::vector<ainet16::TurnPlayerCell> ainet16::Session::my_player_cells() const
{
    vector<TurnPlayerCell> my_pcells;
    int my_nb_pcells = 0;

    for (const TurnPlayer & player : _turn.players)
    {
        if (player.player_id == _player_id)
        {
            my_nb_pcells = player.nb_cells;
            continue;
        }
    }

    my_pcells.reserve(my_nb_pcells);

    for (const TurnPlayerCell & pcell : _turn.pcells)
        if (pcell.player_id == _player_id)
            my_pcells.push_back(pcell);

    return my_pcells;
}

std::vector<ainet16::TurnPlayerCell> ainet16::Session::ennemy_player_cells() const
{
    vector<TurnPlayerCell> ennemy_pcells;
    int ennemy_nb_pcells = 0;

    for (const TurnPlayer & player : _turn.players)
        if (player.player_id != _player_id)
            ennemy_nb_pcells += player.nb_cells;

    ennemy_pcells.reserve(ennemy_nb_pcells);

    for (const TurnPlayerCell & pcell : _turn.pcells)
        if (pcell.player_id != _player_id)
            ennemy_pcells.push_back(pcell);

    return ennemy_pcells;
}

std::vector<ainet16::TurnVirus> ainet16::Session::viruses() const
{
    return _turn.viruses;
}

std::vector<ainet16::TurnPlayer> ainet16::Session::players() const
{
    return _turn.players;
}

bool ainet16::Session::is_connected() const
{
    return _is_connected;
}

bool ainet16::Session::is_logged() const
{
    return _is_logged;
}

bool ainet16::Session::is_player() const
{
    return _is_player;
}


sf::Uint8 ainet16::Session::read_uint8() throw(AINetException)
{
    sf::Uint8 ui8;

    sf::Socket::Status status;
    std::size_t to_receive = 1;
    std::size_t received = 0;

    while (received < to_receive)
    {
        std::size_t received_tmp;

        status = _socket.receive(((char*)&ui8)+received, to_receive - received, received_tmp);
        received += received_tmp;

        if (status != sf::Socket::Done)
        {
            if (status == sf::Socket::Error)
                throw SocketErrorException("Socket error while attempting to read uint8");
            else if (status == sf::Socket::Disconnected)
                throw DisconnectedException("Disconnected while attempting to read uint8");
        }
    }

    return ui8;
}

sf::Uint32 ainet16::Session::read_uint32() throw(AINetException)
{
    sf::Uint32 ui32;

    sf::Socket::Status status;
    std::size_t to_receive = 4;
    std::size_t received = 0;

    while (received < to_receive)
    {
        std::size_t received_tmp;

        status = _socket.receive(((char*)&ui32)+received, to_receive - received, received_tmp);
        received += received_tmp;

        if (status != sf::Socket::Done)
        {
            if (status == sf::Socket::Error)
                throw SocketErrorException("Socket error while attempting to read uint32");
            else if (status == sf::Socket::Disconnected)
                throw DisconnectedException("Disconnected while attempting to read uint32");
        }
    }

    return ui32;
}

sf::Uint64 ainet16::Session::read_uint64() throw(AINetException)
{
    sf::Uint32 ui64;

    sf::Socket::Status status;
    std::size_t to_receive = 8;
    std::size_t received = 0;

    while (received < to_receive)
    {
        std::size_t received_tmp;

        status = _socket.receive(((char*)&ui64)+received, to_receive - received, received_tmp);
        received += received_tmp;

        if (status != sf::Socket::Done)
        {
            if (status == sf::Socket::Error)
                throw SocketErrorException("Socket error while attempting to read uint64");
            else if (status == sf::Socket::Disconnected)
                throw DisconnectedException("Disconnected while attempting to read uint64");
        }
    }

    return ui64;
}

float ainet16::Session::read_float() throw(AINetException)
{
    float f;

    sf::Socket::Status status;
    std::size_t to_receive = 4;
    std::size_t received = 0;

    while (received < to_receive)
    {
        std::size_t received_tmp;

        status = _socket.receive(((char*)&f)+received, to_receive - received, received_tmp);
        received += received_tmp;

        if (status != sf::Socket::Done)
        {
            if (status == sf::Socket::Error)
                throw SocketErrorException("Socket error while attempting to read float");
            else if (status == sf::Socket::Disconnected)
                throw DisconnectedException("Disconnected while attempting to read float");
        }
    }

    return f;
}

string ainet16::Session::read_string() throw(AINetException)
{
    try
    {
        string str;
        sf::Uint32 str_size = read_uint32();

        str.resize(str_size);

        for (unsigned int i = 0; i < str_size; ++i)
            str[i] = char(read_uint8());

        return str;
    }
    catch (const SocketErrorException & e)
    {
        throw SocketErrorException(e.whatstr() + ", while attempting to read string");
    }
    catch (const DisconnectedException & e)
    {
        throw DisconnectedException(e.whatstr() + ", while attempting to read string");
    }
}

ainet16::Position ainet16::Session::read_position() throw(AINetException)
{
    try
    {
        Position p;
        p.x = read_float();
        p.y = read_float();

        return p;
    }
    catch (const SocketErrorException & e)
    {
        throw SocketErrorException(e.whatstr() + ", while attempting to read position");
    }
    catch (const DisconnectedException & e)
    {
        throw DisconnectedException(e.whatstr() + ", while attempting to read position");
    }
}

bool ainet16::Session::read_bool() throw(AINetException)
{
    try
    {
        sf::Uint8 ui8 = read_uint8();
        if (ui8 == 0)
            return false;
        else if (ui8 == 1)
            return false;
        else
            throw AINetException("Bad boolean received: not 0 nor 1");
    }
    catch (const SocketErrorException & e)
    {
        throw SocketErrorException(e.whatstr() + ", while attempting to read bool");
    }
    catch (const DisconnectedException & e)
    {
        throw DisconnectedException(e.whatstr() + ", while attempting to read bool");
    }
}

ainet16::MetaProtocolStamp ainet16::Session::read_stamp() throw(AINetException)
{
    try
    {
        sf::Uint8 ui8 = read_uint8();

        if (ui8 > 9)
            throw AINetException("Bad stamp received: not in [0,9]");

        MetaProtocolStamp stamp = (MetaProtocolStamp) ui8;
        return stamp;
    }
    catch (const SocketErrorException & e)
    {
        throw SocketErrorException(e.whatstr() + ", while attempting to read stamp");
    }
    catch (const DisconnectedException & e)
    {
        throw DisconnectedException(e.whatstr() + ", while attempting to read stamp");
    }
}

void ainet16::Session::send_uint8(sf::Uint8 ui8) throw(AINetException)
{
    sf::Socket::Status status;
    std::size_t to_send = 1;
    std::size_t sent = 0;

    while (sent < to_send)
    {
        std::size_t sent_tmp;

        status = _socket.send(((char*)&ui8)+sent, to_send - sent, sent_tmp);
        sent += sent_tmp;

        if (status != sf::Socket::Done)
        {
            if (status == sf::Socket::Error)
                throw SocketErrorException("Socket error while attempting to send uint8");
            else if (status == sf::Socket::Disconnected)
                throw DisconnectedException("Disconnected while attempting to send uint8");
        }
    }
}

void ainet16::Session::send_uint32(sf::Uint32 ui32) throw(AINetException)
{
    sf::Socket::Status status;
    std::size_t to_send = 4;
    std::size_t sent = 0;

    while (sent < to_send)
    {
        std::size_t sent_tmp;

        status = _socket.send(((char*)&ui32)+sent, to_send - sent, sent_tmp);
        sent += sent_tmp;

        if (status != sf::Socket::Done)
        {
            if (status == sf::Socket::Error)
                throw SocketErrorException("Socket error while attempting to send uint32");
            else if (status == sf::Socket::Disconnected)
                throw DisconnectedException("Disconnected while attempting to send uint32");
        }
    }
}

void ainet16::Session::send_uint64(sf::Uint64 ui64) throw(AINetException)
{
    sf::Socket::Status status;
    std::size_t to_send = 8;
    std::size_t sent = 0;

    while (sent < to_send)
    {
        std::size_t sent_tmp;

        status = _socket.send(((char*)&ui64)+sent, to_send - sent, sent_tmp);
        sent += sent_tmp;

        if (status != sf::Socket::Done)
        {
            if (status == sf::Socket::Error)
                throw SocketErrorException("Socket error while attempting to send uint64");
            else if (status == sf::Socket::Disconnected)
                throw DisconnectedException("Disconnected while attempting to send uint64");
        }
    }
}

void ainet16::Session::send_float(float f) throw(AINetException)
{
    sf::Socket::Status status;
    std::size_t to_send = 4;
    std::size_t sent = 0;

    while (sent < to_send)
    {
        std::size_t sent_tmp;

        status = _socket.send(((char*)&f)+sent, to_send - sent, sent_tmp);
        sent += sent_tmp;

        if (status != sf::Socket::Done)
        {
            if (status == sf::Socket::Error)
                throw SocketErrorException("Socket error while attempting to send float");
            else if (status == sf::Socket::Disconnected)
                throw DisconnectedException("Disconnected while attempting to send float");
        }
    }
}

void ainet16::Session::send_string(const string &s) throw(AINetException)
{
    try
    {
        sf::Uint32 str_size = s.size();
        send_uint32(str_size);

        for (unsigned int i = 0; i < str_size; ++i)
            send_uint8(s[i]);
    }
    catch (const SocketErrorException & e)
    {
        throw SocketErrorException(e.whatstr() + ", while attempting to send string");
    }
    catch (const DisconnectedException & e)
    {
        throw DisconnectedException(e.whatstr() + ", while attempting to send string");
    }
}

void ainet16::Session::send_position(const ainet16::Position &pos) throw(AINetException)
{
    try
    {
        send_float(pos.x);
        send_float(pos.y);
    }
    catch (const SocketErrorException & e)
    {
        throw SocketErrorException(e.whatstr() + ", while attempting to send position");
    }
    catch (const DisconnectedException & e)
    {
        throw DisconnectedException(e.whatstr() + ", while attempting to send position");
    }
}

void ainet16::Session::send_bool(bool b) throw(AINetException)
{
    try
    {
        sf::Uint8 ui8 = (int)b;
        send_uint8(ui8);
    }
    catch (const SocketErrorException & e)
    {
        throw SocketErrorException(e.whatstr() + ", while attempting to send bool");
    }
    catch (const DisconnectedException & e)
    {
        throw DisconnectedException(e.whatstr() + ", while attempting to send bool");
    }
}

void ainet16::Session::send_stamp(ainet16::MetaProtocolStamp stamp) throw(AINetException)
{
    try
    {
        sf::Uint8 ui8 = (int)stamp;
        send_uint8(ui8);
    }
    catch (const SocketErrorException & e)
    {
        throw SocketErrorException(e.whatstr() + ", while attempting to send stamp");
    }
    catch (const DisconnectedException & e)
    {
        throw DisconnectedException(e.whatstr() + ", while attempting to send stamp");
    }
}

string ainet16::Session::stamp_to_string(ainet16::MetaProtocolStamp stamp) const
{
    int stamp_int = (int)stamp;
    if (stamp_int > 9)
        return "INVALID_STAMP";
    else
        return _stamp_to_string_vector[stamp_int];
}

void ainet16::Session::handle_game_ends() throw(AINetException)
{
    // 7	GAME_ENDS		(GDC.size:ui32, GDC)			The server tells the client the game is over
    // Content : (win_player_id:ui32, nb_players:ui32, (player_id:ui32, score:ui64)*nb_players)
    try
    {
        // Metaprotocol
        // The stamp should has been read already before calling this method

        sf::Uint32 gdc_size = read_uint32();
        (void) gdc_size;

        // Game-dependent protocol
        sf::Uint32 win_player_id = read_uint32();
        sf::Uint32 nb_players = read_uint32();

        std::vector<GameEndsPlayer> players;
        players.resize(nb_players);

        for (GameEndsPlayer & player : players)
        {
            player.player_id = read_uint32();
            player.name = read_string();
            player.score = read_uint64();
        }

        throw GameFinishedException(win_player_id, players);
    }
    catch (const SocketErrorException & e)
    {
        throw SocketErrorException(e.whatstr() + ", while attempting to handle_game_ends");
    }
    catch (const DisconnectedException & e)
    {
        throw DisconnectedException(e.whatstr() + ", while attempting to handle_game_endsœ");
    }
}

ainet16::KickException::KickException(const string &what) :
    ainet16::AINetException(what)
{

}

ainet16::DisconnectedException::DisconnectedException(const string &what) :
    ainet16::AINetException(what)
{

}

ainet16::SocketErrorException::SocketErrorException(const string &what) :
    ainet16::AINetException(what)
{

}

ainet16::GameFinishedException::GameFinishedException(int winner_player_id,
                                                      std::vector<ainet16::GameEndsPlayer> players,
                                                      const string &what) :
    AINetException(what),
    _winner_player_id(winner_player_id),
    _players(players)
{

}

int ainet16::GameFinishedException::winner_player_id() const
{
    return _winner_player_id;
}

std::vector<ainet16::GameEndsPlayer> ainet16::GameFinishedException::players() const
{
    return _players;
}
