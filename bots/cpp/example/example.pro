TEMPLATE = app
TARGET = example
INCLUDEPATH += .
LIBS += -lainl16 -lsfml-network -lsfml-system
QMAKE_CXXFLAGS += -std=c++11

# Input
SOURCES += \
    example.cpp

