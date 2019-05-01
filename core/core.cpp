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

const char* errorCodeStrings[] = {
		"SUCCESS_NOTHING",
		"ERROR_PNG_CODEC",
		"ERROR_NO_BATCH_GIVEN",
		"ERROR_NO_STREAM_INFO",
		"WARNING_OPEN_ASCII_FILENAME",
		"ERROR_OPEN_FILE",
		"ERROR_FIND_VIDEO_STREAM",
		"ERROR_ALLOC_CODEC_CONTEXT",
		"ERROR_CONVERT_CODEC_PARAMS",
		"ERROR_OPEN_CODEC",
		"ERROR_INVALID_PIX_FMT",
		"ERROR_INVALID_WIDTH",
		"ERROR_INVALID_HEIGHT",
		"WARNING_FIND_HW_DEVICE_CONFIG",
		"WARNING_CREATE_HW_DEVICE_CONFIG",
		"WARNING_HW_SURFACE_FORMAT",
		"ERROR_SEEK_VIDEO",
		"ERROR_SEND_PACKET",
		"ERROR_ALLOC_INPUT_FRAME",
		"ERROR_DECODE_VIDEO",
		"ERROR_ALLOC_HW_INPUT_FRAME",
		"ERROR_HW_DATA_TRANSFER",
		"ERROR_ALLOC_OUTPUT_FRAME",
		"ERROR_ALLOC_OUTPUT_FRAME_BUFFER",
		"ERROR_SAVE_THUMBNAIL",
		"ERROR_PNG_ENCODER",
		"WARNING_NO_DEVICE_CODEC",
		"ERROR_CUSTOM_FORMAT_CONTEXT",
		"ERROR_CUSTOM_FORMAT_CONTEXT_OPEN",
		"SUCCESS_DONE",
		"ERROR_CODE_000000030",
		"ERROR_CODE_000000031",
		"ERROR_CODE_000000032",
};

const char* errorIndexToString(unsigned int errorIndex) {
	if (errorIndex >= sizeof(errorCodeStrings) / sizeof(const char*))
		return nullptr;
	return errorCodeStrings[errorIndex];
}

void ErrorReader_init(ErrorReader* errorReader, unsigned int errors) {
	errorReader->errors = errors;
	errorReader->position = 0;
}
const char* ErrorReader_next(ErrorReader * errorReader) {
	while (errorReader->errors) {
		bool hasRest = errorReader->errors % 2;
		errorReader->errors /= 2;
		++errorReader->position;
		if (hasRest)
			return errorIndexToString(errorReader->position);
	}
	return nullptr;
}

HWDevices* getHardwareDevices() {
	static HWDevices devices;
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
