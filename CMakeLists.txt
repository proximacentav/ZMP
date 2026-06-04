cmake_minimum_required(VERSION 3.16)
project(MediaPlayer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia)

add_executable(MediaPlayer
    main.cpp
    mainwindow.h mainwindow.cpp
    audiomanager.h audiomanager.cpp
    deviceswidget.h deviceswidget.cpp
    playerwidget.h playerwidget.cpp
    fileswidget.h fileswidget.cpp
    settingswidget.h settingswidget.cpp
)

target_link_libraries(MediaPlayer Qt6::Core Qt6::Widgets Qt6::Multimedia)