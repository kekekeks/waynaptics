cmake_minimum_required(VERSION 3.16)
project(waynaptics-config LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(waynaptics-config
    main.cpp
    mainwindow.cpp
    socket_client.cpp
    config_model.cpp
    double_slider.cpp
    int_slider.cpp
    test_button.cpp
    test_area.cpp
    touchpad_zone_widget.cpp
    touch_subscriber.cpp
    scroll_edge_widget.cpp
    tabs/scrolling_tab.cpp
    tabs/tapping_tab.cpp
    tabs/pointer_tab.cpp
    tabs/sensitivity_tab.cpp
    tabs/clickzones_tab.cpp
)

target_link_libraries(waynaptics-config PRIVATE
    Qt6::Widgets
    Qt6::Network
)

install(TARGETS waynaptics-config RUNTIME DESTINATION bin)
