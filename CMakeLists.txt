# Copyright 2025 Ingemar Hedvall
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.30)
include(CMakePrintHelpers)

if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
    set(USE_VCPKG OFF)
else()
    set(USE_VCPKG ON)
endif()

option(BUILD_SHARED_LIBS "Static libraries are preferred" OFF)
option(BUS_DOC "If doxygen is installed, then build documentation in Release mode" OFF)
option(BUS_TOOLS "Building applications" OFF)
option(BUS_TEST "Building unit test" OFF)
option(BUS_INTERFACE "Build the interface library" ON)


if (BUS_TOOLS OR BUS_TEST)
   set(BUS_INTERFACE ON)
endif()

if(BUS_INTERFACE AND USE_VCPKG)
    list(APPEND VCPKG_MANIFEST_FEATURES "interface")
endif()

if(BUS_TOOLS AND USE_VCPKG)
    list(APPEND VCPKG_MANIFEST_FEATURES "tools")
endif()

project(bus_message_lib
        VERSION 1.0
        DESCRIPTION "Bus MessageLibrary"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_DEBUG_POSTFIX d)

if (BUS_INTERFACE)
    include(script/boost.cmake)
endif()

if (BUS_TOOLS)
    include(script/utillib.cmake)
    include(script/expat.cmake)
endif()

if (BUS_TEST)
    include(script/googletest.cmake)
endif()

if (BUS_DOC)
    include(script/doxygen.cmake)
    include(script/mkdocs.cmake)
endif()

set(BUS_HEADERS
        include/bus/ibusmessage.h
        include/bus/candataframe.h
)

add_library(bus-message-lib
        src/candataframe.cpp include/bus/candataframe.h
        src/ibusmessage.cpp include/bus/ibusmessage.h
        src/ibusmessagequeue.cpp
        include/bus/ibusmessagequeue.h
        src/ibusmessagebroker.cpp
        include/bus/ibusmessagebroker.h
        src/simulatebroker.cpp
        src/simulatebroker.h
        src/simulatequeue.cpp
        src/simulatequeue.h
        src/buslogstream.cpp
        include/bus/buslogstream.h
)

target_include_directories(bus-message-lib PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
       )
target_include_directories(bus-message-lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

cmake_print_properties(TARGETS bus-message-lib PROPERTIES INCLUDE_DIRECTORIES)

if (MSVC)
    target_compile_definitions(bus-message-lib PRIVATE _WIN32_WINNT=0x0A00)
endif ()


if (BUS_TEST)
    enable_testing()
    add_subdirectory(test)
endif ()

if (BUS_INTERFACE)
    add_subdirectory(interface)
endif ()


if (BUS_TOOLS)
    add_subdirectory(bustolistend)
endif()

if (DOXYGEN_FOUND AND BUS_DOC)

if (BUS_DOC)

    execute_process( COMMAND mkdocs build
                     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})


    set(DOXYGEN_RECURSIVE YES)
    set(DOXYGEN_REPEAT_BRIEF NO)
    set(DOXYGEN_PROJECT_NAME "Bus Messages")
    set(DOXYGEN_PROJECT_BRIEF "Bus Message Serialization Library")
    set(DOXYGEN_PROJECT_LOGO mkdocs/img/logo.png)
    set(DOXYGEN_PROJECT_ICON mkdocs/img/favicon.ico)
    set(DOXYGEN_VERSION "1.0")
    set(DOXYGEN_DISABLE_INDEX YES)
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_FULL_SIDEBAR YES)
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/docs)
    set(DOXYGEN_HTML_OUTPUT "html")
    doxygen_add_docs(bus-message-doc ALL
            include/bus doxygen
            COMMENT "Bus Message Interface Library generating documentation")
endif ()
