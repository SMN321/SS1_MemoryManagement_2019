cmake_minimum_required(VERSION 2.8.9)
project(SS1_MemoryManagement_2019)
add_executable(testit testit.c my_alloc.c my_system.c)
target_link_libraries(testit m)