TEMPLATE = app
TARGET = nearest_splitter
INCLUDEPATH += .
LIBS += -lainl16 -lsfml-network -lsfml-system
QMAKE_CXXFLAGS += -std=c++11

# Input
SOURCES += \
    nearest_splitter.cpp

