//
// Created by notoraptor on 23/06/2018.
//

#ifndef VIDEORAPTOR_VIDEO_DETAILS_HPP
#define VIDEORAPTOR_VIDEO_DETAILS_HPP

#include <cstdint>
#include <cstring>
#include <iostream>

struct VideoDetails {
	char* filename;
	char* title;
	char* container_format;
	char* audio_codec;
	char* video_codec;
	int width;
	int height;
	int frame_rate_num;
	int frame_rate_den;
	int sample_rate;
	int64_t duration;
	int64_t duration_time_base;
	int64_t size;
	int64_t bit_rate;
};

inline void initDetails(VideoDetails* videoDetails) {
	videoDetails->filename = nullptr;
	videoDetails->title = nullptr;
	videoDetails->container_format = nullptr;
	videoDetails->audio_codec = nullptr;
	videoDetails->video_codec = nullptr;
	videoDetails->width = 0;
	videoDetails->height = 0;
	videoDetails->frame_rate_num = 0;
	videoDetails->frame_rate_den = 0;
	videoDetails->sample_rate = 0;
	videoDetails->duration = 0;
	videoDetails->duration_time_base = 0;
	videoDetails->size = 0;
	videoDetails->bit_rate = 0;
}

inline void clearDetails(VideoDetails* videoDetails) {
	delete[] videoDetails->filename;
	delete[] videoDetails->title;
	delete[] videoDetails->container_format;
	delete[] videoDetails->audio_codec;
	delete[] videoDetails->video_codec;
}

inline char* copyString(const char* initialString) {
	size_t stringLength = strlen(initialString);
	char* stringCopy = new char[stringLength + 1];
	memcpy(stringCopy, initialString, stringLength + 1);
	return stringCopy;
}

#endif //VIDEORAPTOR_VIDEO_DETAILS_HPP
