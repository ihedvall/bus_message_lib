# Copyright 2025 Ingemar Hedvall
# SPDX-License-Identifier: MIT

project(TestBusMessage
        VERSION 1.0
        DESCRIPTION "Google unit tests for the Bus Message library"
        LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

add_executable(test-bus-message
        test_ibusmessage.cpp
        test_ibusmessagequeue.cpp
        test_ibusmessagebroker.cpp)

target_include_directories(test-bus-message PRIVATE
        ../src ../include
        ${googletest_SOURCE_DIR} )

if (MINGW)
    target_link_options(test-bus-message PRIVATE -static -fstack-protector)
elseif (MSVC)
    target_compile_definitions(test-bus-message PRIVATE -D_WIN32_WINNT=0x0A00)
endif ()

target_link_libraries(test-bus-message PRIVATE bus-message-lib)
target_link_libraries(test-bus-message PRIVATE GTest::gtest GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(test-bus-message)


