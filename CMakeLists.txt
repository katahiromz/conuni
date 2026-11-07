cmake_minimum_required(VERSION 3.10)

project(conuni VERSION 0.1.2 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "conuni is a Win32 library and can only be built on Windows.")
endif()

option(CONUNI_BUILD_TESTS "Build the conuni tests" ON)

#######################################################################
# Library

add_library(conuni STATIC conuni.cpp conuni.h)
target_include_directories(conuni PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

if(MSVC)
    target_compile_options(conuni PRIVATE /W4 /utf-8)
    target_compile_definitions(conuni PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(conuni PRIVATE -Wall -Wextra)
endif()

#######################################################################
# Tests

if(CONUNI_BUILD_TESTS)
    enable_testing()

    # ANSI build (UNICODE not defined)
    add_executable(conuni_test_ansi conuni_test.cpp)
    target_link_libraries(conuni_test_ansi PRIVATE conuni)

    # UNICODE build
    add_executable(conuni_test_unicode conuni_test.cpp)
    target_link_libraries(conuni_test_unicode PRIVATE conuni)
    target_compile_definitions(conuni_test_unicode PRIVATE UNICODE _UNICODE)

    foreach(target conuni_test_ansi conuni_test_unicode)
        if(MSVC)
            target_compile_options(${target} PRIVATE /W4 /utf-8)
            target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
        else()
            target_compile_options(${target} PRIVATE -Wall -Wextra)
        endif()
        add_test(NAME ${target} COMMAND ${target})
    endforeach()
endif()

#######################################################################
# Install

install(TARGETS conuni
        ARCHIVE DESTINATION lib)
install(FILES conuni.h
        DESTINATION include)
