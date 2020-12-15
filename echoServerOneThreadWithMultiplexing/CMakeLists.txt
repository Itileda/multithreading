cmake_minimum_required(VERSION 3.5)

project(echoServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(WITH_POLL "Poll method used" ON)
option(WITH_SELECT "Select method used" OFF)

if(WITH_POLL)
    add_definitions(-DUSE_POLL=ON)
else()
    add_definitions(-DUSE_POLL=OFF)
endif()

if(WITH_SELECT)
    add_definitions(-DUSE_SELECT=ON)
else()
    add_definitions(-DUSE_SELECT=OFF)
endif()

add_executable(echoServer main.cpp)
