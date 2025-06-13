# Copyright 2025 Ingemar Hedvall
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.30)
include(CMakePrintHelpers)

project(bus_message_interface
        VERSION 1.0
        DESCRIPTION "Bus Message Interface Library"
        LANGUAGES CXX)

set(BUS_INTERFACE_HEADERS
     ../include/bus/interface/businterfacefactory.h
)


add_library(bus-message-interface
        src/businterfacefactory.cpp ../include/bus/interface/businterfacefactory.h
        src/sharedmemorybroker.cpp
        src/sharedmemorybroker.h
        src/sharedmemoryqueue.cpp
        src/sharedmemoryqueue.h)

target_include_directories(bus-message-interface PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)

target_include_directories(bus-message-interface PRIVATE ${Boost_INCLUDE_DIRS} )

cmake_print_properties(TARGETS bus-message-interface PROPERTIES INCLUDE_DIRECTORIES)
cmake_print_variables(Boost_INCLUDE_DIRS Boost_INCLUDE_DIR)