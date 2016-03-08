TEMPLATE = app
TARGET = sonic
INCLUDEPATH += .
LIBS += -lainl16 -lsfml-network -lsfml-system
QMAKE_CXXFLAGS += -std=c++11

# Input
SOURCES += \
    sonic.cpp

