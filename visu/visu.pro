#-------------------------------------------------
#
# Project created by QtCreator 2016-02-10T20:07:39
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = visu
TEMPLATE = app
LIBS += -lainetlib16 -lsfml-graphics -lsfml-window -lsfml-network -lsfml-system
QMAKE_CXXFLAGS += -std=c++11 -Wall -Wextra


SOURCES += main.cpp\
        visu.cpp \
    network.cpp \
    cell.cpp \
    test.cpp

HEADERS  += visu.hpp \
    network.hpp \
    structures.hpp \
    cell.hpp \
    test.h
