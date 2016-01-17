#-------------------------------------------------
#
# Project created by QtCreator 2015-03-31T23:59:33
#
#-------------------------------------------------

QT += core network gui

QMAKE_CXXFLAGS = -std=c++14 -Wall -Wextra

TARGET = server
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    server.cpp \
    client.cpp \
    game.cpp \
    tank_game.cpp \
    tank_map.cpp \
    cli.cpp \
    cell_game.cpp

HEADERS += \
    server.hpp \
    client.hpp \
    protocol.hpp \
    game.hpp \
    tank_game.hpp \
    tank_map.hpp \
    cli.hpp \
    cell_game.hpp
