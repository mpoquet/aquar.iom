TEMPLATE = app
TARGET = afk
INCLUDEPATH += .
LIBS += -lainetlib16 -lsfml-network -lsfml-system
QMAKE_CXXFLAGS += -std=c++11

# Input
SOURCES += \
    nearest.cpp

