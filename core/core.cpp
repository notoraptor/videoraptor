//
// Created by notoraptor on 27/05/2018.
//
extern "C" {
#include <libavcodec/avcodec.h>
};
#include "utils.hpp"
#include "VideoInfo.hpp"
#include "VideoThumbnail.hpp"
#include "VideoRaptorInfo.hpp"

// Doing nothing (silent log).
void customCallback(void* avClass, int level, const char* fmt, va_list vl) {}

HWDevices* getHardwareDevices() {
	static HWDevices devices;
	static bool initialized = false;
	if (!initialized) {
		// Initializations.
		// Set custom callback
		// TODO (is it still useful?).
		av_log_set_callback(customCallback);
		initialized = true;
	}
	return &devices;
}

void VideoRaptorInfo_init(VideoRaptorInfo* videoRaptorInfo) {
	HWDevices* devices = getHardwareDevices();
	videoRaptorInfo->hardwareDevicesCount = devices->countDeviceTypes();
	videoRaptorInfo->hardwareDevicesNames = nullptr;
	if (videoRaptorInfo->hardwareDevicesCount) {
		const char* separator = ", ";
		videoRaptorInfo->hardwareDevicesNames = new char[devices->getStringRepresentationLength(separator)];
		devices->getStringRepresentation(videoRaptorInfo->hardwareDevicesNames, separator);
	}
}

void VideoRaptorInfo_clear(VideoRaptorInfo* videoRaptorInfo) {
	delete[] videoRaptorInfo->hardwareDevicesNames;
}

void VideoThumbnail_init(VideoThumbnail* videoThumbnail, const char* filename, const char* thumbnailFolder, const char* thumbnailName) {
	videoThumbnail->filename = filename;
	videoThumbnail->thumbnailFolder = thumbnailFolder;
	videoThumbnail->thumbnailName = thumbnailName;
	VideoReport_init(&videoThumbnail->report);
}

void VideoInfo_init(VideoInfo* videoInfo, const char* filename) {
	videoInfo->filename = filename;
	videoInfo->title = nullptr;
	videoInfo->container_format = nullptr;
	videoInfo->audio_codec = nullptr;
	videoInfo->video_codec = nullptr;
	videoInfo->width = 0;
	videoInfo->height = 0;
	videoInfo->frame_rate_num = 0;
	videoInfo->frame_rate_den = 0;
	videoInfo->sample_rate = 0;
	videoInfo->duration = 0;
	videoInfo->duration_time_base = 0;
	videoInfo->size = 0;
	videoInfo->bit_rate = 0;
	VideoReport_init(&videoInfo->report);
}

void VideoInfo_clear(VideoInfo* videoInfo) {
	// Field `filename` is not modified.
	// All potentially allocated fields are cleared.
	delete[] videoInfo->title;
	delete[] videoInfo->container_format;
	delete[] videoInfo->audio_codec;
	delete[] videoInfo->video_codec;
}

bool VideoInfo_error(VideoInfo* videoInfo, unsigned int errorCode, const char* errorDetail) {
	return VideoReport_error(&videoInfo->report, errorCode, errorDetail);
}
