QT += core gui widgets multimedia multimediawidgets sql

qtHaveModule(texttospeech) {
    QT += texttospeech
    DEFINES += HAS_TEXTTOSPEECH
}

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

!wasm {
    SOURCES += \
        client/facedetector.cpp \
        client/facealigner.cpp \
        client/facerecognizer.cpp \
        client/facedatabase.cpp \
        client/facemanagerdialog.cpp

    HEADERS += \
        client/facedetector.h \
        client/facealigner.h \
        client/facerecognizer.h \
        client/facedatabase.h \
        client/facemanagerdialog.h

    # OpenCV (set OPENCV_DIR environment variable or modify path below)
    OPENCV_DIR = $$(OPENCV_DIR)
    isEmpty(OPENCV_DIR) {
        OPENCV_DIR = C:/opencv/opencv/build
    }
    INCLUDEPATH += $$OPENCV_DIR/include
    LIBS += -L$$OPENCV_DIR/x64/vc16/lib -lopencv_world490

    # ONNX Runtime (set ONNXRUNTIME_DIR environment variable or modify path below)
    ONNXRUNTIME_DIR = $$(ONNXRUNTIME_DIR)
    isEmpty(ONNXRUNTIME_DIR) {
        ONNXRUNTIME_DIR = C:/onnxruntime
    }
    INCLUDEPATH += $$ONNXRUNTIME_DIR/include
    LIBS += -L$$ONNXRUNTIME_DIR/lib -lonnxruntime
}

wasm {
    QMAKE_LFLAGS += -s TOTAL_MEMORY=268435456 -s ALLOW_MEMORY_GROWTH=1
    QMAKE_LFLAGS += -s EXPORTED_RUNTIME_METHODS=[ccall,cwrap]
    QMAKE_LFLAGS += -s EXPORTED_FUNCTIONS=[_main,_malloc,_free]
}