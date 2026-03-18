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
        options.cpp)
target_include_directories(waynaptics PRIVATE include fakeclude ../../external/xf86-input-synaptics/include  ${LIBEVDEV_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})
target_link_libraries(waynaptics PRIVATE ${LIBEVDEV_LIBRARIES} ${GLIB_LDFLAGS})