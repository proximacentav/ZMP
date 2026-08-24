cmake_minimum_required(VERSION 3.16)
project(ZMP_Linux_bin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(WIN32)
    # DBus отсутствует на Windows (MPRIS — только Linux)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Svg Network OpenGLWidgets)
else()
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Svg Network DBus OpenGLWidgets)
endif()

# --- Опциональные зависимости (на Windows могут отсутствовать в sysroot) ---
function(zmp_find_opt name header lib out_def)
    find_path(${name}_INCLUDE_DIR NAMES ${header}
        PATHS /usr/include /usr/local/include "$ENV{ZMP_MINGW_ROOT}/include")
    find_library(${name}_LIBRARY NAMES ${lib}
        PATHS /usr/lib /usr/local/lib "$ENV{ZMP_MINGW_ROOT}/lib")
    if(${name}_INCLUDE_DIR AND ${name}_LIBRARY)
        set(${name}_FOUND TRUE PARENT_SCOPE)
    else()
        set(${name}_FOUND FALSE PARENT_SCOPE)
        add_compile_definitions(${out_def})
        message(STATUS "${name} not found -> building WITHOUT ${name}")
    endif()
endfunction()

zmp_find_opt(PROJECTM  libprojectM/projectM.hpp projectM   ZMP_NO_PROJECTM)
zmp_find_opt(TAGLIB    taglib/tag.h             tag        ZMP_NO_TAGLIB)
zmp_find_opt(SOUNDTOUCH soundtouch/SoundTouch.h SoundTouch ZMP_NO_SOUNDTOUCH)

find_library(CRYPT_LIBRARY NAMES crypt)
if(CRYPT_LIBRARY)
    list(APPEND EXTRA_LIBS ${CRYPT_LIBRARY})
endif()

if(WIN32 AND NOT TAGLIB_FOUND)
    message(WARNING "TagLib missing: metadata features disabled")
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
    res.qrc
    third_party/kiss_fft.c
)
if(NOT WIN32)
    target_sources(ZMP_Linux_bin PRIVATE mpriscontroller.h mpriscontroller.cpp)
endif()

target_include_directories(ZMP_Linux_bin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)
if(PROJECTM_FOUND)
    target_include_directories(ZMP_Linux_bin PRIVATE ${PROJECTM_INCLUDE_DIR})
endif()
if(TAGLIB_FOUND)
    target_include_directories(ZMP_Linux_bin PRIVATE ${TAGLIB_INCLUDE_DIR})
endif()
if(SOUNDTOUCH_FOUND)
    target_include_directories(ZMP_Linux_bin PRIVATE ${SOUNDTOUCH_INCLUDE_DIR})
endif()

target_link_libraries(ZMP_Linux_bin
    Qt6::Core Qt6::Widgets Qt6::Multimedia Qt6::Svg Qt6::Network
    $<$<PLATFORM_ID:Linux>:Qt6::DBus>
    Qt6::OpenGLWidgets
    ${EXTRA_LIBS})
if(PROJECTM_FOUND)
    target_link_libraries(ZMP_Linux_bin ${PROJECTM_LIBRARY})
endif()
if(TAGLIB_FOUND)
    target_link_libraries(ZMP_Linux_bin ${TAGLIB_LIBRARY})
endif()
if(SOUNDTOUCH_FOUND)
    target_link_libraries(ZMP_Linux_bin ${SOUNDTOUCH_LIBRARY})
endif()

if(WIN32)
    set_target_properties(ZMP_Linux_bin PROPERTIES OUTPUT_NAME "zmp")
    target_compile_definitions(ZMP_Linux_bin PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()