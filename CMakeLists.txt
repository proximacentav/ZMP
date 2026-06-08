cmake_minimum_required(VERSION 3.16)
project(MediaPlayer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_PREFIX_PATH "/usr/lib/qt6" ${CMAKE_PREFIX_PATH})

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Svg)

# bass
set(BASS_ROOT_DIR "/usr")
find_path(BASS_INCLUDE_DIR NAMES bass.h PATHS ${BASS_ROOT_DIR}/include /usr/include)
find_library(BASS_LIBRARY NAMES bass PATHS ${BASS_ROOT_DIR}/lib /usr/lib)
find_library(BASS_FX_LIBRARY NAMES bass_fx PATHS ${BASS_ROOT_DIR}/lib /usr/lib)

# tagLib
find_package(PkgConfig REQUIRED)
pkg_check_modules(TAGLIB REQUIRED taglib)

add_executable(MediaPlayer
    main.cpp
    audiomanager.h audiomanager.cpp
    deviceswidget.h deviceswidget.cpp
    playerwidget.h playerwidget.cpp
    fileswidget.h fileswidget.cpp
    equalizerwidget.h equalizerwidget.cpp
    settingswidget.h settingswidget.cpp
    mainwindow.h mainwindow.cpp
    metadataextractor.h metadataextractor.cpp
    trackinfowidget.h trackinfowidget.cpp
    res.qrc
)

target_include_directories(MediaPlayer PRIVATE
    ${BASS_INCLUDE_DIR}
    ${TAGLIB_INCLUDE_DIRS}
)

target_link_libraries(MediaPlayer
    Qt6::Core Qt6::Widgets Qt6::Multimedia Qt6::Svg
    ${BASS_LIBRARY} ${BASS_FX_LIBRARY}
    ${TAGLIB_LIBRARIES}
)
