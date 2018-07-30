//
// Created by notoraptor on 27/05/2018.
//
extern "C" {
#include <libavcodec/avcodec.h>
};
#include "AppError.hpp"
#include "Output.hpp"
#include "initVideoRaptor.hpp"

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

HWDevices* initVideoRaptor(std::basic_ostream<char>& out) {
	static HWDevices devices(out);
	static bool initialized = false;
	if (!initialized) {
		// Initializations.
		av_log_set_callback(customCallback);
		// Load output image codec (for video thumbnails).
		imageCodec = avcodec_find_encoder(AV_CODEC_ID_PNG);
		if (!imageCodec) {
			AppError(out).write("PNG codec not found.");
			return nullptr;
		}
		initialized = true;
	}
	return &devices;
}
