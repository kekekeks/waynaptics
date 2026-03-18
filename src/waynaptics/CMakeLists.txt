cmake_minimum_required(VERSION 3.25)
project(waynaptics)
#include (${CMAKE_ROOT}/Modules/FindX11.cmake)
find_package(PkgConfig REQUIRED)
pkg_search_module(LIBEVDEV REQUIRED libevdev)
pkg_search_module(GLIB REQUIRED glib-2.0)

set(CMAKE_CXX_STANDARD 20)
add_definitions(-w)
add_definitions(-DBUILD_EVENTCOMM)
add_executable(waynaptics main.cpp
        ../../external/xf86-input-synaptics/src/synaptics.c
        ../../external/xf86-input-synaptics/src/eventcomm.c
        ../../external/xf86-input-synaptics/src/synproto.c
        ../../external/xf86-input-synaptics/src/properties.c

        fakeclude/xorg-server.h
        fakeclude/xf86.h
        fakeclude/xf86Xinput.h
        fakeclude/exevents.h
        fakeclude/xserver-properties.h
        fakeclude/ptrveloc.h
        fakeclude/xisb.h
        fakeclude/shim.h
        supportlib.cpp
        include/synshared.h
        mainloop.cpp
        include/mainloop.h
        valuators.c
        fakeclude/xf86Module.h
        atoms.cpp
        options.cpp
        device_init.cpp
        device_mgmt.cpp
        xi_properties.cpp
        output_backend.cpp
        event_posting.cpp
        synclient_loader.cpp
        ptrveloc.cpp
        include/device_init.h
        include/output_backend.h
        include/xi_properties.h)
target_include_directories(waynaptics PRIVATE include fakeclude ../../external/xf86-input-synaptics/include ../../external/xf86-input-synaptics/src ${LIBEVDEV_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})
target_link_libraries(waynaptics PRIVATE ${LIBEVDEV_LIBRARIES} ${GLIB_LDFLAGS})

# Standalone utility to dump actual synaptics XI properties from running X server
find_package(X11 REQUIRED)
pkg_search_module(XI REQUIRED xi)
add_executable(waynaptics-dump dump_synaptics.c)
target_include_directories(waynaptics-dump PRIVATE ../../external/xf86-input-synaptics/include)
target_link_libraries(waynaptics-dump PRIVATE ${X11_LIBRARIES} ${XI_LIBRARIES})