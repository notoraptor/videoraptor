//
// Created by notoraptor on 15/04/2019.
//

#ifndef VIDEORAPTOR_VIDEOLOG_HPP
#define VIDEORAPTOR_VIDEOLOG_HPP

#include <cstdio>

#define ERROR_DETAIL_MAX_LENGTH 64

struct VideoLog {
	unsigned int errors;
	char errorDetail[ERROR_DETAIL_MAX_LENGTH];
};

inline void VideoLog_init(VideoLog* videoLog) {
	videoLog->errors = 0;
	videoLog->errorDetail[0] = '\0';
}

inline bool VideoLog_error(VideoLog* videoLog, unsigned int errorCode, const char* errorDetail = nullptr) {
	videoLog->errors |= errorCode;
	if (errorDetail)
		snprintf(videoLog->errorDetail, ERROR_DETAIL_MAX_LENGTH, errorDetail);
	return false;
}

#include <cstdint>
#include <iostream>

#endif //VIDEORAPTOR_VIDEOLOG_HPP
