requires(qtHaveModule(core))
requires(qtHaveModule(gui))
requires(qtHaveModule(widgets))
requires(qtHaveModule(multimedia))
requires(qtHaveModule(multimediawidgets))
requires(qtHaveModule(sql))

lessThan(QT_MAJOR_VERSION, 6): error(This project requires Qt 6.6 or newer)
lessThan(QT_MINOR_VERSION, 6): error(This project requires Qt 6.6 or newer)

QT += core gui widgets multimedia multimediawidgets sql

qtHaveModule(texttospeech) {
    QT += texttospeech
    DEFINES += HAS_TEXTTOSPEECH
}

TARGET = BntechEyeFriend
TEMPLATE = app

CONFIG += c++17

# Add CONFIG+=static_exe when using a static Qt build.
static_exe {
    CONFIG += static
    DEFINES += QT_STATIC
    msvc:QMAKE_CFLAGS_RELEASE += /MT
    msvc:QMAKE_CXXFLAGS_RELEASE += /MT
    msvc:QMAKE_LFLAGS_RELEASE += /INCREMENTAL:NO
}

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

RESOURCES += \
    client/resources.qrc

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
    # 编译优化 - 升级到 O3 级别
    QMAKE_CXXFLAGS += -O3 -ffast-math -fno-exceptions -fno-rtti -fomit-frame-pointer
    QMAKE_CFLAGS += -O3 -ffast-math -fno-exceptions -fno-rtti -fomit-frame-pointer

    # 链接优化 - 启用更严格的优化
    QMAKE_LFLAGS += -s TOTAL_MEMORY=134217728 -s ALLOW_MEMORY_GROWTH=0  # 128MB 静态内存
    QMAKE_LFLAGS += -s ASYNCIFY=1 -lidbfs.js
    QMAKE_LFLAGS += -s EXPORTED_RUNTIME_METHODS=[ccall,cwrap]
    QMAKE_LFLAGS += -s EXPORTED_FUNCTIONS=[_main,_malloc,_free]
    QMAKE_LFLAGS += -O3  # 链接优化升级到 O3
    QMAKE_LFLAGS += -sNO_DISABLE_EXCEPTION_CATCHING=0
    QMAKE_LFLAGS += -sDISABLE_EXCEPTION_CATCHING=1
    QMAKE_LFLAGS += -sASSERTIONS=0
    QMAKE_LFLAGS += -sWASM=1
    QMAKE_LFLAGS += -sMODULARIZE=0
    QMAKE_LFLAGS += -sEXPORT_ALL=0
    QMAKE_LFLAGS += -sSTRICT=0
    QMAKE_LFLAGS += -flto  # 链接时优化

    # 新增高级优化选项
    QMAKE_LFLAGS += -s ALIASING_FUNCTION_POINTERS=1
    QMAKE_LFLAGS += -s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE='["malloc","free"]'
    QMAKE_LFLAGS += -s ELIMINATE_DUPLICATE_FUNCTIONS=1
    QMAKE_LFLAGS += -s MEMORY_GROWTH_LINEAR=0
    QMAKE_LFLAGS += -s ONLY_MY_CODE=1
    QMAKE_LFLAGS += -s WASM_BIGINT=0
    QMAKE_LFLAGS += -s TOTAL_STACK=5MB  # 减少栈空间
}