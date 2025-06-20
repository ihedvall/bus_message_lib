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
        src/sharedmemoryqueue.h
        src/tcpmessagebroker.cpp
        src/tcpmessagebroker.h
        src/tcpmessageconnection.cpp
        src/tcpmessageconnection.h
        src/tcpmessageclient.cpp
        src/tcpmessageclient.h
        src/sharedmemoryserver.cpp
        src/sharedmemoryserver.h
        src/sharedmemorytxrxqueue.cpp
        src/sharedmemorytxrxqueue.h
        src/sharedmemoryclient.cpp
        src/sharedmemoryclient.h
        src/tcpmessageserver.cpp
        src/tcpmessageserver.h
)

target_include_directories(bus-message-interface PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)

target_include_directories(bus-message-interface PRIVATE ${Boost_INCLUDE_DIRS} )

if (MSVC)
  target_compile_definitions(bus-message-interface PRIVATE _WIN32_WINNT=0x0A00)
endif ()

cmake_print_properties(TARGETS bus-message-interface PROPERTIES INCLUDE_DIRECTORIES)
cmake_print_variables(Boost_INCLUDE_DIRS Boost_INCLUDE_DIR)