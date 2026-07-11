cmake_minimum_required(VERSION 3.16)
project(ZMP_Linux_bin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Svg Network DBus OpenGLWidgets)

# BASS
find_path(BASS_INCLUDE_DIR NAMES bass.h PATHS /usr/include /usr/local/include)
find_library(BASS_LIBRARY NAMES bass PATHS /usr/lib /usr/local/lib)
find_library(BASS_FX_LIBRARY NAMES bass_fx PATHS /usr/lib /usr/local/lib)

# projectM
find_path(PROJECTM_INCLUDE_DIR NAMES libprojectM/projectM.hpp PATHS /usr/include /usr/local/include)
find_library(PROJECTM_LIBRARY NAMES projectM PATHS /usr/lib /usr/local/lib)

# TagLib
find_path(TAGLIB_INCLUDE_DIR NAMES taglib/tag.h PATHS /usr/include /usr/local/include)
find_library(TAGLIB_LIBRARY NAMES tag libtag PATHS /usr/lib /usr/local/lib)

if(NOT TAGLIB_LIBRARY)
    message(FATAL_ERROR "TagLib not found. Install it: sudo pacman -S taglib (Arch) or apt-get install libtag1-dev (Debian)")
endif()

add_executable(ZMP_Linux_bin
    main.cpp
    mainwindow.h mainwindow.cpp
    audiomanager.h audiomanager.cpp
    deviceswidget.h deviceswidget.cpp
    playerwidget.h playerwidget.cpp
    playbackcontrolwidget.h playbackcontrolwidget.cpp
    equalizerwidget.h equalizerwidget.cpp
    visualizationwidget.h visualizationwidget.cpp
    settingswidget.h settingswidget.cpp
    fileswidget.h fileswidget.cpp
    playlistswidget.h playlistswidget.cpp
    iconbutton.h
    miniplayerbar.h miniplayerbar.cpp
    mpriscontroller.h mpriscontroller.cpp
    res.qrc
)

target_include_directories(ZMP_Linux_bin PRIVATE
    ${BASS_INCLUDE_DIR}
    ${PROJECTM_INCLUDE_DIR}
    ${TAGLIB_INCLUDE_DIR}
)

target_link_libraries(ZMP_Linux_bin
    Qt6::Core Qt6::Widgets Qt6::Multimedia Qt6::Svg Qt6::Network Qt6::DBus Qt6::OpenGLWidgets
    ${BASS_LIBRARY} ${BASS_FX_LIBRARY}
    ${PROJECTM_LIBRARY}
    ${TAGLIB_LIBRARY}
)