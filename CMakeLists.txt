cmake_minimum_required(VERSION 3.10)
project(videoraptor)

set(CMAKE_CXX_STANDARD 11)
include_directories(${PROJECT_SOURCE_DIR}/../ffmpeg-4.0-win64-dev/include)
include_directories(${PROJECT_SOURCE_DIR})
add_executable(videoraptor videoraptor.cpp lodepng.h lodepng.cpp)
