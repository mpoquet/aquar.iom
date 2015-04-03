#-------------------------------------------------
#
# Project created by QtCreator 2015-03-31T23:59:33
#
#-------------------------------------------------

QT += core network
QT -= gui

QMAKE_CXXFLAGS = -std=c++14 -Wall -Wextra

TARGET = server
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    server.cpp \
    client.cpp \
    logger.cpp \
    game.cpp

HEADERS += \
    server.hpp \
    client.hpp \
    logger.hpp \
    protocol.hpp \
    game.hpp
