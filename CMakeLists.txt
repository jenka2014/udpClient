cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

# set the project name
project(udpClient VERSION 2.0 LANGUAGES CXX)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

add_executable(udpClient 
    src/udpClient.cpp
    src/md5.cpp 
    include/date.h
    include/md5.h)

set_target_properties(udpClient PROPERTIES LINKER_LANGUAGE CXX)


