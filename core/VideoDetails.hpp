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

inline void printDetails(VideoDetails* videoDetails) {
	std::cout << "BEGIN DETAILS" << std::endl;
	if (videoDetails->filename)
		std::cout << "\tfilename          : " << videoDetails->filename << std::endl;
	if (videoDetails->title)
		std::cout << "\ttitle             : " << videoDetails->title << std::endl;
	if (videoDetails->container_format)
		std::cout << "\tcontainer_format  : " << videoDetails->container_format << std::endl;
	if (videoDetails->audio_codec)
		std::cout << "\taudio_codec       : " << videoDetails->audio_codec << std::endl;
	if (videoDetails->video_codec)
		std::cout << "\tvideo_codec       : " << videoDetails->video_codec << std::endl;
	std::cout << "\twidth             : " << videoDetails->width << std::endl;
	std::cout << "\theight            : " << videoDetails->height << std::endl;
	std::cout << "\tframe_rate_num    : " << videoDetails->frame_rate_num << std::endl;
	std::cout << "\tframe_rate_den    : " << videoDetails->frame_rate_den << std::endl;
	std::cout << "\tsample_rate       : " << videoDetails->sample_rate << std::endl;
	std::cout << "\tduration          : " << videoDetails->duration << std::endl;
	std::cout << "\tduration_time_base: " << videoDetails->duration_time_base << std::endl;
	std::cout << "\tsize              : " << videoDetails->size << std::endl;
	std::cout << "\tbit_rate          : " << videoDetails->bit_rate << std::endl;
	std::cout << "END DETAILS" << std::endl;
}

inline char* copyString(const char* initialString) {
	size_t stringLength = strlen(initialString);
	char* stringCopy = new char[stringLength + 1];
	memcpy(stringCopy, initialString, stringLength + 1);
	return stringCopy;
}

#endif //VIDEORAPTOR_VIDEO_DETAILS_HPP
