cmake_minimum_required(VERSION 3.16)
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
        valuators.c
        synaptics_private_state.c
        fakeclude/xf86Module.h
        atoms.cpp
        options.cpp
        device_init.cpp
        device_mgmt.cpp
        xi_properties.cpp
        output_backend.cpp
        event_posting.cpp
        synclient_loader.cpp
        config_socket.cpp
        ptrveloc.cpp
        include/device_init.h
        include/output_backend.h
        include/accel.h
        include/xi_properties.h
        include/synclient_loader.h
        include/config_socket.h)

# fakeclude provides X server shim headers for the C driver code only
target_include_directories(waynaptics PRIVATE
        include
        $<$<COMPILE_LANGUAGE:C>:${CMAKE_CURRENT_SOURCE_DIR}/fakeclude>
        ../../external/xf86-input-synaptics/include
        ../../external/xf86-input-synaptics/src
        ${LIBEVDEV_INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS})
target_link_libraries(waynaptics PRIVATE ${LIBEVDEV_LIBRARIES} ${GLIB_LDFLAGS})

install(TARGETS waynaptics RUNTIME DESTINATION bin COMPONENT daemon)
install(FILES ${CMAKE_SOURCE_DIR}/dist/waynaptics.service
        DESTINATION lib/systemd/system COMPONENT daemon)