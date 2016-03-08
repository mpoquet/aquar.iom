#-------------------------------------------------
#
# Project created by QtCreator 2016-02-10T20:07:39
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = visu
TEMPLATE = app
LIBS += -lainl16 -lsfml-graphics -lsfml-window -lsfml-network -lsfml-system
QMAKE_CXXFLAGS += -std=c++11 -Wall -Wextra


SOURCES += main.cpp\
        visu.cpp \
    cell.cpp \
    test.cpp

HEADERS  += visu.hpp \
    cell.hpp \
    test.h
