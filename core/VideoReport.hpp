//
// Created by notoraptor on 15/04/2019.
//

#ifndef VIDEORAPTOR_VIDEOREPORT_HPP
#define VIDEORAPTOR_VIDEOREPORT_HPP

#include <cstdio>
#include "errorCodes.hpp"

#define ERROR_DETAIL_MAX_LENGTH 64

struct VideoReport {
	unsigned int errors;
	char errorDetail[ERROR_DETAIL_MAX_LENGTH];
};

inline void VideoReport_init(VideoReport* report) {
	report->errors = 0;
	report->errorDetail[0] = '\0';
}

inline bool VideoReport_error(VideoReport* report, unsigned int errorCode, const char* errorDetail = nullptr) {
	report->errors |= errorCode;
	if (errorDetail)
		snprintf(report->errorDetail, ERROR_DETAIL_MAX_LENGTH, errorDetail);
	return false;
}

inline bool VideoReport_setDone(VideoReport* report, bool done) {
	if (done) {
		report->errors |= SUCCESS_DONE;
	} else if (report->errors & SUCCESS_DONE) {
		report->errors ^= SUCCESS_DONE;
	}
	return done;
}

inline bool VideoReport_isDone(VideoReport* report) {
	return report->errors & SUCCESS_DONE;
}

inline bool VideoReport_hasError(VideoReport* videoReport) {
	return videoReport->errors && !(videoReport->errors & SUCCESS_DONE);
}

inline void VideoReport_print(VideoReport* report) {
	printError(report->errors);
	if (report->errorDetail[0] != '\0')
		std::cout << "ERROR_DETAIL: " << report->errorDetail << std::endl;
}

#include <cstdint>
#include <iostream>

#endif //VIDEORAPTOR_VIDEOREPORT_HPP
