#include "ainetlib16.hpp"

using namespace std;

ainet16::Exception::Exception(const string & what)
{
    _what = what;
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

}

ainet16::Session::~Session()
{

}

void ainet16::Session::connect(string address, int port) throw(Exception)
{
    if (port < 0 || port > 65535)
        throw Exception("Bad port");

    sf::Socket::Status status = _socket.connect(sf::IpAddress(address), port);

    if (status != sf::Socket::Done)
        throw Exception("Impossible to connect on socket");
}

void ainet16::Session::login_player(string name) throw(Exception)
{
    // Sending LOGIN_PLAYER message
    sf::Packet packet;

    packet << sf::Uint8(LOGIN_PLAYER);
    packet << sf::Uint32(name.size());

    for (unsigned int i = 0; i < name.size(); ++i)
        packet << sf::Uint8(name[i]);

    send_packet(packet);

    // Waiting for LOGIN_ACK
    sf::Uint8 stamp_ui8 = read_uint8();
    MetaProtocolStamp stamp = (MetaProtocolStamp) stamp_ui8;

    if (stamp == MetaProtocolStamp::LOGIN_ACK)
    {
        _is_player = true;
        return;
    }
    else if (stamp == MetaProtocolStamp::KICK)
    {
        string kick_reason = read_string();
        throw KickException(kick_reason);
    }
    else
        throw Exception("Invalid response to LOGIN_PLAYER, received_stamp=" + std::to_string(stamp_ui8));
}

void ainet16::Session::login_visu(string name) throw(Exception)
{
    // Sending LOGIN_VISU message
    sf::Packet packet;

    packet << sf::Uint8(LOGIN_VISU);
    packet << sf::Uint32(name.size());

    for (unsigned int i = 0; i < name.size(); ++i)
        packet << sf::Uint8(name[i]);

    send_packet(packet);

    // Waiting for LOGIN_ACK
    sf::Uint8 stamp_ui8 = read_uint8();
    MetaProtocolStamp stamp = (MetaProtocolStamp) stamp_ui8;

    if (stamp == MetaProtocolStamp::LOGIN_ACK)
    {
        _is_player = false;
        return;
    }
    else if (stamp == MetaProtocolStamp::KICK)
    {
        string kick_reason = read_string();
        throw KickException(kick_reason);
    }
    else
        throw Exception("Invalid response to LOGIN_VISU, received_stamp=" + std::to_string(stamp_ui8));
}

void ainet16::Session::wait_for_welcome() throw(Exception)
{
    // Metaprotocol
    sf::Uint8 stamp_ui8 = read_uint8();
    MetaProtocolStamp stamp = (MetaProtocolStamp) stamp_ui8;

    if (stamp != MetaProtocolStamp::WELCOME)
    {
        if (stamp == MetaProtocolStamp::KICK)
        {
            string kick_reason = read_string();
            throw KickException(kick_reason);
        }
        else
            throw Exception("Invalid stamp received while waiting for WELCOME, received_stamp=" + std::to_string(stamp_ui8));
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
    _welcome.parameters.radius_factor = read_float();
    _welcome.parameters.max_cells_per_player = read_uint32();
    _welcome.parameters.mass_loss_per_frame = read_float();
    _welcome.parameters.base_cell_speed = read_float();
    _welcome.parameters.speed_loss_factor = read_float();
    _welcome.parameters.virus_mass = read_float();
    _welcome.parameters.virus_creation_mass_loss = read_float();
    _welcome.parameters.virus_max_split = read_uint32();
    _welcome.parameters.nb_starting_cells_per_player = read_uint32();
    _welcome.parameters.player_cells_starting_mass = read_uint32();
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
}

void ainet16::Session::send_actions(const ainet16::Actions &actions) throw(Exception)
{
    // Compute game-dependent content size
    sf::Uint32 gdc_size = 0;

    gdc_size += 4 + actions._move_actions.size() * (4 + 4*2);
    gdc_size += 4 + actions._divide_actions.size() * (4 + 4*2 + 4);
    gdc_size += 4 + actions._create_virus_actions.size() * (4 + 4*2);
    gdc_size += 1;

    sf::Packet packet;

    // Metaprotocol header
    packet << sf::Uint8(MetaProtocolStamp::TURN_ACK)
           << sf::Uint32(gdc_size)
           << sf::Uint32(_last_received_turn);

    // Game-dependent content
    packet << sf::Uint32(actions._move_actions.size());
    for (const MoveAction & action : actions._move_actions)
        packet << sf::Uint32(action.pcell_id)
               << action.position.x
               << action.position.y;

    packet << sf::Uint32(actions._divide_actions.size());
    for (const DivideAction & action : actions._divide_actions)
        packet << sf::Uint32(action.pcell_id)
               << action.position.x
               << action.position.y
               << action.mass;

    packet << sf::Uint32(actions._create_virus_actions.size());
    for (const CreateVirusAction & action : actions._create_virus_actions)
        packet << sf::Uint32(action.pcell_id)
               << action.position.x
               << action.position.y;

    packet << sf::Uint8(actions._will_surrender);
    send_packet(packet);
}

void ainet16::Session::wait_for_next_turn() throw(Exception)
{
    // Metaprotocol
    sf::Uint8 stamp_ui8 = read_uint8();
    MetaProtocolStamp stamp = (MetaProtocolStamp) stamp_ui8;

    if (stamp != MetaProtocolStamp::TURN)
    {
        if (stamp == MetaProtocolStamp::KICK)
        {
            string kick_reason = read_string();
            throw KickException(kick_reason);
        }
        else
            throw Exception("Invalid stamp received while waiting for TURNR, received_stamp=" + std::to_string(stamp_ui8));
    }

    sf::Uint32 gdc_size = read_uint32();
    (void) gdc_size;

    _last_received_turn = read_uint32();

    // Game-dependent protocol
    sf::Uint32 nb_initial_ncells = read_uint32();
    _turn.initial_ncells.resize(nb_initial_ncells);
    for (TurnInitialNeutralCell & ncell : _turn.initial_ncells)
        ncell.remaining_turns_before_apparition = read_uint32();

    sf::Uint32 nb_non_initial_ncells = read_uint32();
    _turn.non_initial_ncells.resize(nb_non_initial_ncells);
    for (TurnNonInitialNeutralCell & ncell : _turn.non_initial_ncells)
    {
        ncell.ncell_id = read_uint32();
        ncell.mass = read_float();
        ncell.position = read_position();
    }

    sf::Uint32 nb_viruses = read_uint32();
    _turn.viruses.resize(nb_viruses);
    for (TurnVirus & virus : _turn.viruses)
    {
        virus.id = read_uint32();
        virus.position = read_position();
    }

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

    sf::Uint32 nb_players = read_uint32();
    _turn.players.resize(nb_players);
    for (TurnPlayer & player : _turn.players)
    {
        player.player_id = read_uint32();
        player.nb_cells = read_uint32();
        player.mass = read_float();
        player.score = read_uint64();
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

std::vector<ainet16::NeutralCell> ainet16::Session::all_neutral_cells() const
{

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


sf::Uint8 ainet16::Session::read_uint8() throw(Exception)
{
    sf::Uint8 ui8;

    sf::Socket::Status status;
    std::size_t received;

    status = _socket.receive(&ui8, 1, received);
    if (status != sf::Socket::Done || received != 1)
        throw Exception("Cannot read uint8");

    return ui8;
}

sf::Uint32 ainet16::Session::read_uint32() throw(Exception)
{
    sf::Uint32 ui32;

    sf::Socket::Status status;
    std::size_t received;

    status = _socket.receive(&ui32, 4, received);
    if (status != sf::Socket::Done || received != 4)
        throw Exception("Cannot read uint32");

    return ui32;
}

sf::Uint64 ainet16::Session::read_uint64() throw(Exception)
{
    sf::Uint64 ui64;

    sf::Socket::Status status;
    std::size_t received;

    status = _socket.receive(&ui64, 8, received);
    if (status != sf::Socket::Done || received != 8)
        throw Exception("Cannot read uint64");

    return ui64;
}

float ainet16::Session::read_float() throw(Exception)
{
    float f;

    sf::Socket::Status status;
    std::size_t received;

    status = _socket.receive(&f, 4, received);
    if (status != sf::Socket::Done || received != 4)
        throw Exception("Cannot read float");

    return f;
}

string ainet16::Session::read_string() throw(Exception)
{
    string str;
    sf::Uint32 str_size = read_uint32();

    str.resize(str_size);

    for (unsigned int i = 0; i < str_size; ++i)
        str[i] = char(read_uint8());

    return str;
}

ainet16::Position ainet16::Session::read_position() throw(Exception)
{
    Position p;
    p.x = read_float();
    p.y = read_float();

    return p;
}

bool ainet16::Session::read_bool() throw(Exception)
{
    sf::Uint8 ui8 = read_uint8();
    if (ui8 == 0)
        return false;
    else if (ui8 == 1)
        return false;
    else
        throw Exception("Bad boolean received: not 0 nor 1");
}

void ainet16::Session::send_packet(sf::Packet &packet) throw(Exception)
{
    sf::Socket::Status status = _socket.send(packet);

    if (status != sf::Socket::Done)
        throw Exception("Impossible to send LOGIN_PLAYER");
}


ainet16::KickException::KickException(const string &what) :
    ainet16::Exception(what)
{

}

ainet16::DisconnectedException::DisconnectedException(const string &what) :
    ainet16::Exception(what)
{
    // todo: detect disconnections and throw this exception when needed
    // todo: good partial readings
}
