cmake_minimum_required(VERSION 3.5)

project(echoServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(echoServer main.cpp)
