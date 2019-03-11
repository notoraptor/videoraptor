//
// Created by notoraptor on 23/06/2018.
//

#ifndef VIDEORAPTOR_VIDEO_DETAILS_HPP
#define VIDEORAPTOR_VIDEO_DETAILS_HPP

#include <cstdint>
#include <cstring>
#include <iostream>

#define ERROR_DETAIL_MAX_LENGTH 64

struct VideoErrors {
	unsigned int errors;
	char errorDetail[ERROR_DETAIL_MAX_LENGTH];
};

struct VideoDetails {
	const char* filename;
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
	bool done;
	VideoErrors errors;
};

struct VideoThumbnailInfo {
	const char* filename;
	const char* thumbnailName;
	VideoErrors errors;
};

inline void VideoErrors_init(VideoErrors* videoErrors) {
	videoErrors->errors = 0;
	videoErrors->errorDetail[0] = '\0';
}

inline void VideoDetails_init(VideoDetails* videoDetails) {
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
	videoDetails->done = false;
	VideoErrors_init(&videoDetails->errors);
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


inline bool videoRaptorError(VideoErrors* videoErrors, unsigned int errorCode, const char* errorDetail = nullptr) {
	videoErrors->errors |= errorCode;
	if (errorDetail) {
		snprintf(videoErrors->errorDetail, ERROR_DETAIL_MAX_LENGTH, errorDetail);
	}
	return false;
}

inline bool videoRaptorError(VideoDetails* videoDetails, unsigned int errorCode, const char* errorDetail = nullptr) {
	return videoRaptorError(&videoDetails->errors, errorCode, errorDetail);
}

#endif //VIDEORAPTOR_VIDEO_DETAILS_HPP
