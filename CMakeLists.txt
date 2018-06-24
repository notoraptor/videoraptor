cmake_minimum_required(VERSION 3.10)
project(videoraptor)

set(CMAKE_CXX_STANDARD 11)
include_directories(${PROJECT_SOURCE_DIR}/../ffmpeg-4.0-win64-dev/include)
include_directories(${PROJECT_SOURCE_DIR})
add_executable(videoraptor
        core/AppError.hpp
        core/common.cpp
        core/common.hpp
        core/Output.hpp
        core/StringEncoder.hpp
        core/VideoDetails.hpp
        lodepng/lodepng.cpp
        lodepng/lodepng.h
        videoRaptor/videoRaptor.cpp
        videoRaptorBatch/videoRaptorBatch.cpp
        videoRaptorBatch/videoRaptorBatch.hpp
        videoRaptorOnce/videoRaptorOnce.cpp
        videoRaptorOnce/videoRaptorOnce.hpp
        )
