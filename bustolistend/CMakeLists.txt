# Copyright 2022 Ingemar Hedvall
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)
project(bustolistend
        VERSION 1.0
        DESCRIPTION "Daemon  that run bus message listen servers"
        LANGUAGES CXX)

add_executable(bustolistend src/main.cpp
        src/bustolisten.cpp
        src/bustolisten.h)

target_include_directories(bustolistend PRIVATE ${Boost_INCLUDE_DIRS})
target_compile_definitions(bustolistend PRIVATE XML_STATIC)

if (MSVC)
    target_compile_definitions(bustolistend PRIVATE _WIN32_WINNT=0x0A00)
endif ()

target_link_libraries(bustolistend PRIVATE util)
target_link_libraries(bustolistend PRIVATE bus-message-lib)
target_link_libraries(bustolistend PRIVATE EXPAT::EXPAT)
target_link_libraries(bustolistend PRIVATE ${Boost_LIBRARIES})
target_link_libraries(bustolistend PRIVATE bus-message-interface)
target_link_libraries(bustolistend PRIVATE sago::platform_folders)

if (WIN32)
    target_link_libraries(bustolistend PRIVATE bcrypt)
    target_link_libraries(bustolistend PRIVATE ws2_32)
    target_link_libraries(bustolistend PRIVATE mswsock)
endif ()