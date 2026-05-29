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
)

set(SOURCES
    client/main.cpp
    client/mainwindow.cpp
    client/camerawidget.cpp
    client/volumeslider.cpp
    client/pulsedots.cpp
    client/whisperclient.cpp
)

set(HEADERS
    client/mainwindow.h
    client/camerawidget.h
    client/volumeslider.h
    client/pulsedots.h
    client/whisperclient.h
)

if(EMSCRIPTEN)
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
    
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
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
    
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Multimedia
        Qt5::MultimediaWidgets
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/client
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
