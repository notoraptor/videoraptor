cmake_minimum_required(VERSION 3.10)
project(videoraptor)

set(CMAKE_CXX_STANDARD 11)
include_directories(${PROJECT_SOURCE_DIR}/ffmpeg/include)
add_executable(videoraptor videoraptor.cpp tutorial01.c)
