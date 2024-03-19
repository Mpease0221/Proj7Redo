cmake_minimum_required(VERSION 3.22)
project(Proj7Redo)

set(CMAKE_CXX_STANDARD 17)

add_executable(Proj7Redo main.cpp)
target_link_libraries(Proj7Redo -lglut -lGL -lGLEW)
