TEMPLATE = app
TARGET = move_to_corner
INCLUDEPATH += .
LIBS += -lainl16 -lsfml-network -lsfml-system
QMAKE_CXXFLAGS += -std=c++11

# Input
SOURCES += \
    move_to_corner.cpp

