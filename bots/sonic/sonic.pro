TEMPLATE = app
TARGET = sonic
INCLUDEPATH += .
LIBS += -lainetlib16 -lsfml-network -lsfml-system
QMAKE_CXXFLAGS += -std=c++11

# Input
SOURCES += \
    sonic.cpp

