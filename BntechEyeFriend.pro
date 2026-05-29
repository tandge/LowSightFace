QT += core gui widgets multimedia multimediawidgets

TARGET = BntechEyeFriend
TEMPLATE = app

CONFIG += c++17

msvc:QMAKE_CXXFLAGS += /utf-8

SOURCES += \
    client/main.cpp \
    client/mainwindow.cpp \
    client/camerawidget.cpp \
    client/pulsedots.cpp \
    client/whisperclient.cpp

HEADERS += \
    client/mainwindow.h \
    client/camerawidget.h \
    client/pulsedots.h \
    client/whisperclient.h

FORMS += \
    client/mainwindow.ui

INCLUDEPATH += client

wasm {
    QMAKE_LFLAGS += -s TOTAL_MEMORY=268435456 -s ALLOW_MEMORY_GROWTH=1
    QMAKE_LFLAGS += -s EXPORTED_RUNTIME_METHODS=[ccall,cwrap]
    QMAKE_LFLAGS += -s EXPORTED_FUNCTIONS=[_main,_malloc,_free]
}