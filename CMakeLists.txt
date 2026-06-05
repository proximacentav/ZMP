cmake_minimum_required(VERSION 3.16)
project(MediaPlayer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Svg)

# Путь к библиотекам BASS (если они установлены в нестандартное место)
set(BASS_ROOT_DIR "/usr")

find_path(BASS_INCLUDE_DIR
    NAMES bass.h
    PATHS ${BASS_ROOT_DIR}/include /usr/include /usr/local/include
)

find_library(BASS_LIBRARY
    NAMES bass libbass
    PATHS ${BASS_ROOT_DIR}/lib ${BASS_ROOT_DIR}/lib64 /usr/lib /usr/local/lib
)

# <<< НОВЫЙ БЛОК ДЛЯ bass_fx >>>
find_library(BASS_FX_LIBRARY
    NAMES bass_fx libbass_fx
    PATHS /usr/lib /usr/local/lib ${BASS_ROOT_DIR}/lib ${BASS_ROOT_DIR}/x64
)

if(NOT BASS_INCLUDE_DIR OR NOT BASS_LIBRARY)
    message(FATAL_ERROR "BASS library not found! Please install libbass and set BASS_ROOT_DIR correctly.")
endif()

# Подсказка, если bass_fx не найден
if(NOT BASS_FX_LIBRARY)
    message(WARNING "BASS FX library not found. Equalizer will be disabled.")
endif()

add_executable(MediaPlayer
    main.cpp
    audiomanager.h audiomanager.cpp
    deviceswidget.h deviceswidget.cpp # пока что бета
    playerwidget.h playerwidget.cpp
    fileswidget.h fileswidget.cpp
    equalizerwidget.h equalizerwidget.cpp
    settingswidget.h settingswidget.cpp
    mainwindow.h mainwindow.cpp
    res.qrc   
)

target_link_libraries(MediaPlayer
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Svg
    ${BASS_LIBRARY}
    ${BASS_FX_LIBRARY}
)
