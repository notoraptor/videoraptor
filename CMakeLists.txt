cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
project(videoRaptor VERSION 0.0.1 DESCRIPTION "A shared library to extract videos info using ffmpeg.")
include_directories(${PROJECT_SOURCE_DIR})
include_directories($ENV{CPATH})
link_directories($ENV{LIBRARY_PATH})
set(COMMON_SOURCES
        core/compatWindows.hpp
        core/core.cpp
        core/FileHandle.hpp
        core/HWDevices.hpp
        core/videoRaptorInit.hpp
        core/Output.hpp
        core/Stream.hpp
        core/ThumbnailContext.hpp
        core/unicode.hpp
        core/utils.hpp
        core/Video.hpp
        core/VideoDetails.hpp
        lib/lodepng/lodepng.cpp
        lib/lodepng/lodepng.h
        lib/utf/utf.hpp
        videoRaptorBatch/videoRaptorBatch.cpp
        videoRaptorBatch/videoRaptorBatch.hpp
        )
set(TEST_SOURCES
        test.cpp
        core/errorCodes.hpp)
add_library(videoRaptorBatch MODULE ${COMMON_SOURCES} core/errorCodes.hpp)
add_executable(test ${COMMON_SOURCES} ${TEST_SOURCES})

target_link_libraries(videoRaptorBatch avcodec avformat avutil swscale)
set_target_properties(videoRaptorBatch PROPERTIES PREFIX "")
set_target_properties(videoRaptorBatch PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(videoRaptorBatch PROPERTIES SOVERSION 1)
install(TARGETS videoRaptorBatch LIBRARY DESTINATION $ENV{BIN_PATH})
target_link_libraries(test avcodec avformat avutil swscale)

