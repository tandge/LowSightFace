cmake_minimum_required(VERSION 3.14)
project(BntechEyeFriend VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt5 WASM configuration
find_package(Qt5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Multimedia
    MultimediaWidgets
    Sql
)

if(NOT EMSCRIPTEN)
    find_package(OpenCV REQUIRED)
    find_path(ONNXRUNTIME_INCLUDE_DIR NAMES onnxruntime_cxx_api.h
              PATHS $ENV{ONNXRUNTIME_DIR}/include
                    ${CMAKE_SOURCE_DIR}/third_party/onnxruntime/include)
    find_library(ONNXRUNTIME_LIB NAMES onnxruntime
                 PATHS $ENV{ONNXRUNTIME_DIR}/lib
                       ${CMAKE_SOURCE_DIR}/third_party/onnxruntime/lib)
    if(NOT ONNXRUNTIME_INCLUDE_DIR OR NOT ONNXRUNTIME_LIB)
        message(FATAL_ERROR "ONNX Runtime not found. Set ONNXRUNTIME_DIR environment variable or place it in third_party/onnxruntime.")
    endif()
endif()

set(SOURCES
    client/main.cpp
    client/mainwindow.cpp
    client/camerawidget.cpp
    client/pulsedots.cpp
    client/whisperclient.cpp
)

set(HEADERS
    client/mainwindow.h
    client/camerawidget.h
    client/pulsedots.h
    client/whisperclient.h
)

# Qt Designer .ui files (AUTOUIC 会自动调用 uic 生成 ui_*.h)
set(FORMS
    client/mainwindow.ui
)

if(NOT EMSCRIPTEN)
    list(APPEND SOURCES
        client/facedetector.cpp
        client/facealigner.cpp
        client/facerecognizer.cpp
        client/facedatabase.cpp
        client/facemanagerdialog.cpp
    )
    list(APPEND HEADERS
        client/facedetector.h
        client/facealigner.h
        client/facerecognizer.h
        client/facedatabase.h
        client/facemanagerdialog.h
    )
endif()

if(EMSCRIPTEN)
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${FORMS})

    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Multimedia
        Qt5::MultimediaWidgets
    )

    # WASM-specific linker flags
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "-s WASM=1 -s USE_QT5=1 -s TOTAL_MEMORY=268435456 -s ALLOW_MEMORY_GROWTH=1 --preload-file ${CMAKE_SOURCE_DIR}/assets@/assets -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] -s EXPORTED_FUNCTIONS=['_main','_malloc','_free'] -s ASYNCIFY=1 -s ASYNCIFY_IGNORE_INDIRECT=1"
    )
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${FORMS})

    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Multimedia
        Qt5::MultimediaWidgets
        Qt5::Sql
        ${OpenCV_LIBS}
        ${ONNXRUNTIME_LIB}
    )

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${OpenCV_INCLUDE_DIRS}
        ${ONNXRUNTIME_INCLUDE_DIR}
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/client
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
