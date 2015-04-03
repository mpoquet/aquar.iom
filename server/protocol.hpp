#pragma once

#include <cstdint>

/*
 * All messages start with
 * A stamp is sent on every message beginning. Stamps are sent as uint8
 *
 * All integers are sent in little-endian
    * uint8 : a 8-bit unsigned integer
    * uint32 : 32-bit unsigned integer
 * char:(latin1:uint8)
 * string: (size:uint32, c(0):char, c(1):char, ... c(size-1):char)
 *  example: "string" -> (6,'s','t','r','i','n','g')
 */

enum class Stamp : std::uint8_t
{
    // These messages are handled by the generic network
    LOGIN_PLAYER=0      //! Client(U) -> Server. Content=(nick:string)
    ,LOGIN_VISU         //! Client(U) -> Server. Content=(nick:string)
    ,LOGIN_ACK          //! Server -> Client(U->P). Content=()
    ,KICK               //! Server -> Client(*). Content=(reason:string)
    ,LOGOUT             //! Server -> Client((P|V)->U). Content=()

    // Client sockets emit signals corresponding to these stamps.
    // These signals must be handled by the game.
    ,WELCOME        //! Server -> Client(P|V). Content=(contentSize:uint32,GAME_DEPENDENT)
    ,GAME_STARTS    //! Server -> Client(P|V). Content=(contentSize:uint32,GAME_DEPENDENT)
    ,TURN           //! Server -> Client(P|V). Content=(contentSize:uint32,turn:uint32,GAME_DEPENDENT)
    ,TURN_ACK       //! Client(P|V) -> Server. Content=(contentSize:uint32,turn:uint32,GAME_DEPENDENT)
};
