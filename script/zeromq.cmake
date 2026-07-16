# Copyright 2021 Ingemar Hedvall
# SPDX-License-Identifier: MIT
include(CMakePrintHelpers)

if (NOT ZeroMQ_FOUND)
    if (COMP_DIR)
        set(ZeroMQ_ROOT ${COMP_DIR}/libzmq/latest)
    endif()

    find_package(ZeroMQ REQUIRED CONFIG)
endif()

if (NOT cppzmq_FOUND)
    if (COMP_DIR)
        set(cppzmq_ROOT ${COMP_DIR}/cppzmq/latest)
    endif()

    find_package(cppzmq REQUIRED CONFIG)
endif()

cmake_print_variables( ZeroMQ_FOUND
        ZeroMQ_VERSION
        ZeroMQ_INCLUDE_DIR
        ZeroMQ_LIBRARY
        ZeroMQ_STATIC_LIBRARY)
cmake_print_properties(TARGETS libzmq libzmq-static
                       PROPERTIES INTERFACE_COMPILE_DEFINITIONS
                                  INTERFACE_INCLUDE_DIRECTORIES
                                  INTERFACE_LINK_LIBRARIES
                                  LOCATION)

cmake_print_variables( cppzmq_FOUND
        cppzmq_VERSION
        cppzmq_INCLUDE_DIR)
cmake_print_properties(TARGETS cppzmq cppzmq-static
        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
        INTERFACE_LINK_LIBRARIES)