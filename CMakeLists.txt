cmake_minimum_required(VERSION 3.16)
project(ZMP_Linux_bin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Svg Network DBus OpenGLWidgets)

# projectM
find_path(PROJECTM_INCLUDE_DIR NAMES libprojectM/projectM.hpp PATHS /usr/include /usr/local/include)
find_library(PROJECTM_LIBRARY NAMES projectM PATHS /usr/lib /usr/local/lib)

# TagLib
find_path(TAGLIB_INCLUDE_DIR NAMES taglib/tag.h PATHS /usr/include /usr/local/include)
find_library(TAGLIB_LIBRARY NAMES tag libtag PATHS /usr/lib /usr/local/lib)

# SoundTouch (скорость/питч)
find_path(SOUNDTOUCH_INCLUDE_DIR NAMES soundtouch/SoundTouch.h PATHS /usr/include /usr/local/include)
find_library(SOUNDTOUCH_LIBRARY NAMES SoundTouch PATHS /usr/lib /usr/local/lib)

find_library(CRYPT_LIBRARY NAMES crypt)
if(CRYPT_LIBRARY)
    list(APPEND EXTRA_LIBS ${CRYPT_LIBRARY})
endif()

if(NOT TAGLIB_LIBRARY)
    message(FATAL_ERROR "TagLib not found. Install it: sudo pacman -S taglib (Arch) or apt-get install libtag1-dev (Debian)")
endif()

add_executable(ZMP_Linux_bin
    main.cpp
    translator.h translator.cpp
    mainwindow.h mainwindow.cpp
    audiomanager.h audiomanager.cpp
    deviceswidget.h deviceswidget.cpp
    playerwidget.h playerwidget.cpp
    playbackcontrolwidget.h playbackcontrolwidget.cpp
    equalizerwidget.h equalizerwidget.cpp
    visualizationwidget.h visualizationwidget.cpp
    settingswidget.h settingswidget.cpp
    ftpclient.h ftpclient.cpp
    fileswidget.h fileswidget.cpp
    jamendowidget.h jamendowidget.cpp
    playlistswidget.h playlistswidget.cpp
    depsmanager.h depsmanager.cpp
    partitionsdialog.h partitionsdialog.cpp
    aboutdialog.h aboutdialog.cpp
    audiomodecontroller.h audiomodecontroller.cpp
    devicemanagerdialog.h devicemanagerdialog.cpp
    zmpinstaller.h zmpinstaller.cpp
    singleinstance.h
    cliplayer.h cliplayer.cpp
    iconbutton.h
    miniplayerbar.h miniplayerbar.cpp
    mpriscontroller.h mpriscontroller.cpp
    res.qrc
    third_party/kiss_fft.c
)

target_include_directories(ZMP_Linux_bin PRIVATE
    ${PROJECTM_INCLUDE_DIR}
    ${TAGLIB_INCLUDE_DIR}
    ${SOUNDTOUCH_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_link_libraries(ZMP_Linux_bin
    Qt6::Core Qt6::Widgets Qt6::Multimedia Qt6::Svg Qt6::Network Qt6::DBus Qt6::OpenGLWidgets
    ${PROJECTM_LIBRARY}
    ${TAGLIB_LIBRARY}
    ${SOUNDTOUCH_LIBRARY}
    ${EXTRA_LIBS}
)