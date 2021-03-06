//
// Created by notoraptor on 15/04/2019.
//

#ifndef VIDEORAPTOR_VIDEOREPORT_HPP
#define VIDEORAPTOR_VIDEOREPORT_HPP

#include <cstdio>
#include "errorCodes.hpp"
#include "ErrorReader.hpp"

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

inline void VideoReport_print(VideoReport* report) {
	ErrorReader errorReader;
	ErrorReader_init(&errorReader, report->errors);
	while (const char* errorString = ErrorReader_next(&errorReader))
		std::cout << errorString << std::endl;
	if (report->errorDetail[0] != '\0')
		std::cout << "ERROR_DETAIL: " << report->errorDetail << std::endl;
}

extern "C" {
	bool VideoReport_isDone(VideoReport* report);
	bool VideoReport_hasError(VideoReport* videoReport);
	bool VideoReport_hasDeviceError(VideoReport* videoReport);
}

#endif //VIDEORAPTOR_VIDEOREPORT_HPP
