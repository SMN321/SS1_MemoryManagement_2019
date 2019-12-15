cmake_minimum_required(VERSION 3.10)
project(testumgebung)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(testumgebung
        Makefile
        my_alloc.c
        my_alloc.h
        my_system.c
        my_system.h
        testit.c)
