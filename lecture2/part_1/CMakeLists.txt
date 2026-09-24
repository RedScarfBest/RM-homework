cmake_minimum_required(VERSION 3.16)

project(LECTURE_2)

include_directories(part_1/includes part_2/ex3)

add_executable(cmake_learn
    part_1/src/cmake_learn.cpp
    part_1/tools/hello.cpp
)