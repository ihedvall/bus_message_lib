# Copyright 2025 Ingemar Hedvall
# SPDX-License-Identifier: MIT

project(TestBusMessage
        VERSION 1.0
        DESCRIPTION "Google unit tests for the Bus Message library"
        LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

add_executable(test-bus-message
        src/test_ibusmessage.cpp
        src/test_ibusmessagequeue.cpp
        src/test_ibusmessagebroker.cpp
        src/test_buslogstream.cpp
        src/test_simulatebroker.cpp
        src/test_littlebuffer.cpp
        src/test_candataframe.cpp
        src/test_factory.cpp
        src/test_sharedmemorybroker.cpp
        src/test_tcpmessagebroker.cpp
        src/test_sharedmemoryserver.cpp
        src/test_tcpmessageserver.cpp)

target_include_directories(test-bus-message PRIVATE
        ../src ../include
        ${googletest_SOURCE_DIR} )

if (MINGW)
    target_link_options(test-bus-message PRIVATE -static -fstack-protector)
elseif (MSVC)
    target_compile_definitions(test-bus-message PRIVATE -D_WIN32_WINNT=0x0A00)
endif ()

target_link_libraries(test-bus-message PRIVATE bus-message-lib)
target_link_libraries(test-bus-message PRIVATE bus-message-interface)
target_link_libraries(test-bus-message PRIVATE GTest::gtest GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(test-bus-message)


