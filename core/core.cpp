//
// Created by notoraptor on 27/05/2018.
//
extern "C" {
#include <libavcodec/avcodec.h>
};
#include "Output.hpp"
#include "videoRaptorInit.hpp"
#include "errorCodes.hpp"

const AVCodec* imageCodec = nullptr;
const std::string emptyString;

char* Output::flush() {
	delete[] str;
	std::string s = oss.str();
	oss.str(emptyString);
	str = new char[s.length() + 1];
	memcpy(str, s.data(), s.length());
	str[s.length()] = '\0';
	return str;
}

// Doing nothing (silent log).
void customCallback(void* avClass, int level, const char* fmt, va_list vl) {}

#define INIT_STATUS_NO -1
#define INIT_STATUS_BAD 0
#define INIT_STATUS_GOOD 1

int videoRaptorInit(HWDevices** output) {
	static HWDevices devices;
	static int initStatus = INIT_STATUS_NO;
	static int initError = ERROR_CODE_OK;
	if (initStatus == INIT_STATUS_NO) {
		// Initializations.
		// Set custom callback (is it still useful?).
		av_log_set_callback(customCallback);
		// Load output image codec (for video thumbnails).
		imageCodec = avcodec_find_encoder(AV_CODEC_ID_PNG);
		if (imageCodec) {
			initStatus = INIT_STATUS_GOOD;
		} else {
			initStatus = INIT_STATUS_BAD;
			initError = ERROR_PNG_CODEC;
		}
	}
	if (initStatus == INIT_STATUS_GOOD && output)
		*output = &devices;
	return initError;
}
