cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
project(videoRaptor VERSION 0.0.1 DESCRIPTION "A shared library to extract videos info using ffmpeg.")
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
include_directories(${PROJECT_SOURCE_DIR})
include_directories($ENV{CPATH})
link_directories($ENV{LIBRARY_PATH})
set(COMMON_SOURCES
        alignment/alignment.cpp
        alignment/alignment.hpp
        core/compatWindows.hpp
        core/core.cpp
        core/errorCodes.hpp
        core/ErrorReader.hpp
        core/FileHandle.hpp
        core/HWDevices.hpp
        core/Stream.hpp
        core/ThumbnailContext.hpp
        core/unicode.hpp
        core/utils.hpp
        core/Video.hpp
        core/VideoInfo.hpp
        core/VideoRaptorInfo.hpp
        core/VideoReport.hpp
        core/VideoThumbnail.hpp
        lib/lodepng/lodepng.cpp
        lib/lodepng/lodepng.h
        lib/utf/utf.hpp
        videoRaptorBatch/videoRaptorBatch.cpp
        videoRaptorBatch/videoRaptorBatch.hpp
        )
set(TEST_SOURCES
        test.cpp)
add_library(videoRaptorBatch MODULE ${COMMON_SOURCES})
add_executable(test ${COMMON_SOURCES} ${TEST_SOURCES})

target_link_libraries(videoRaptorBatch avcodec avformat avutil swscale -static-libgcc -static-libstdc++ Threads::Threads)
set_target_properties(videoRaptorBatch PROPERTIES PREFIX "")
set_target_properties(videoRaptorBatch PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(videoRaptorBatch PROPERTIES SOVERSION 1)
install(TARGETS videoRaptorBatch LIBRARY DESTINATION $ENV{BIN_PATH})
target_link_libraries(test avcodec avformat avutil swscale -static-libgcc -static-libstdc++ Threads::Threads)

