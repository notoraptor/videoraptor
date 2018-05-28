//
// Created by notoraptor on 27/05/2018.
//
#include "common.hpp"

const AVPixelFormat PIXEL_FMT = AV_PIX_FMT_RGBA;
const AVCodec* imageCodec = nullptr;
const char* currentFilename = nullptr;
bool initialized = false;

void customCallback(void* avcl, int level, const char* fmt, va_list vl) {
	if (level < av_log_get_level() && currentFilename) {
		char currentMessage[2048] = {};
		vsnprintf(currentMessage, sizeof(currentMessage), fmt, vl);
		for (char& character: currentMessage) {
			if (character == '\r' || character == '\n')
				character = ' ';
		}
		std::cout << "#VIDEO_WARNING[" << currentFilename << "]" << currentMessage << std::endl;
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
