cmake_minimum_required(VERSION 3.16)
project(waynaptics_root)

option(BUILD_CONFIG_TOOL "Build the Qt6 configuration utility" ON)

add_subdirectory(src/waynaptics)
if(BUILD_CONFIG_TOOL)
    add_subdirectory(src/waynaptics-config)
endif()
