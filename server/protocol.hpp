#pragma once

#include <cstdint>

/*
Network protocol description

0. Introduction
    This protocol is based on TCP and uses binary data.

1. Data types
    name    meaning
    --------------------------------------------------------------------
    ui8		8-bit unsigned integer
    ui32	32-bit unsigned integer (little-endian)
    char 	one Latin-1 character (sent as ui8)
    string	(s.size:ui32, s[0]:char, s[1]:char, ..., s[s.size-1]:char)

2. Messages
    Every message is composed of a type (sent as ui8) directly followed by a content.
    The content of some messages is game-dependent (GDC stands for GAME_DEPENDENT_CONTENT).
    The following table show all possible messages.

    id	type 			content 							meaning
    ------------------------------------------------------------------------------------------------------------
    0	LOGIN_PLAYER	(nickname:string)				The client wants to log in as a player
    1	LOGIN_VISU		(nickname:string)				The client wants to log in as a visualizer
    2	LOGIN_ACK		()								The server accepts the client logging
    3	LOGOUT			()								The server tells the client it had been logged out
    4	KICK			(reason:string)					The server explains why the client is about to be kicked
    5	WELCOME			(GDC.size:ui32, GDC)			The server sends some welcome information to the client
    6	GAME_STARTS		(GDC.size:ui32, GDC)			The server tells the client the game is about to start
    7	GAME_ENDS		(GDC.size:ui32, GDC)			The server tells the client the game is over
    8	TURN 			(GDC.size:ui32, turn:ui32, GDC)	The server announces the client that a game round begins
    9	TURN_ACK		(GDC.size:ui32, turn:ui32, GDC)	The client answers the game round

3. Expected client behaviour
    The client behaviour must match a certain behaviour.
    This behaviour is described by a graph in file 'client_behaviour.png'
    Please note that some edges are not displayed in it. These edges are showed in 'client_behaviour_full.png'.

    The graph nodes are client states. Graph edges can be:
        - "? TYPE": the client receives a message of type TYPE
        - "! TYPE": the client sends a message of type TYPE

    A client that do not respect the protocol is kicked:
        1. A message of type KICK is sent to him,
        2. The socket is closed.
*/

enum class Stamp : std::uint8_t
{
    // These messages are handled by the generic network
    LOGIN_PLAYER=0      //! Client(U) -> Server. Content=(nick:string)
    ,LOGIN_VISU         //! Client(U) -> Server. Content=(nick:string)
    ,LOGIN_ACK          //! Server -> Client(U->(P|V)). Content=()
    ,LOGOUT             //! Server -> Client((P|V)->U). Content=()
    ,KICK               //! Server -> Client(*). Content=(reason:string)

    // Client sockets emit signals corresponding to these stamps.
    // These signals must be handled by the game.
    ,WELCOME        //! Server -> Client(P|V). Content=(size(GAME_DEPENDENT):uint32,GAME_DEPENDENT)
    ,GAME_STARTS    //! Server -> Client(P|V). Content=(size(GAME_DEPENDENT):uint32,GAME_DEPENDENT)
    ,GAME_ENDS      //! Server -> Client(P|V). Content=(size(GAME_DEPENDENT):uint32,GAME_DEPENDENT)
    ,TURN           //! Server -> Client(P|V). Content=(size(GAME_DEPENDENT):uint32,turn:uint32,GAME_DEPENDENT)

    ,TURN_ACK       //! Client(P|V) -> Server. Content=(size(GAME_DEPENDENT):uint32,turn:uint32,GAME_DEPENDENT)
};
