//
// Created by notoraptor on 27/05/2018.
//
#include <iostream>
#include <mutex>
#include <core/Output.hpp>
#include "common.hpp"

#ifdef WIN32
const char separator = '\\';
const char otherSeparator = '/';
#else
const char separator = '/';
const char otherSeparator = '\\';
#endif

const char* const digits = "0123456789ABCDEF";

const AVPixelFormat PIXEL_FMT = AV_PIX_FMT_RGBA;
const AVCodec* imageCodec = nullptr;
const char* currentFilename = nullptr;
bool initialized = false;

std::mutex loggerMutex;

std::ostringstream& videoRaptorLogger(char** outputString) {
	static Output output;
	if (outputString)
		*outputString = output.flush();
	return output.os;
}

void lockLogger() {
	loggerMutex.lock();
}

void unlockLogger() {
	loggerMutex.unlock();
}

void customCallback(void* avcl, int level, const char* fmt, va_list vl) {
	if (level < av_log_get_level() && currentFilename) {
		char currentMessage[2048] = {};
		vsnprintf(currentMessage, sizeof(currentMessage), fmt, vl);
		for (char& character: currentMessage) {
			if (character == '\r' || character == '\n')
				character = ' ';
		}
		loggerMutex.lock();
		videoRaptorLogger(nullptr) << "#VIDEO_WARNING[" << currentFilename << "]" << currentMessage << std::endl;
		loggerMutex.unlock();
	}
}

HWDevices* initVideoRaptor(std::basic_ostream<char>& out) {
	static HWDevices devices(out);
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
