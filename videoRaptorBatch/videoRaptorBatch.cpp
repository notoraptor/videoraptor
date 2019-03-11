//
// Created by notoraptor on 27/05/2018.
//

#include <string>
#include <fstream>
#include <iostream>
#include <core/videoRaptorInit.hpp>
#include <core/Output.hpp>
#include <core/Video.hpp>
#include <core/errorCodes.hpp>
#include "videoRaptorBatch.hpp"

inline bool getVideoThumbnails(HWDevices &devices, VideoThumbnailInfo* videoThumbnailInfo, const char *thFolder) {
	for (size_t i = 0; i < devices.available.size(); ++i) {
		size_t indexToUse = (devices.indexUsed + i) % devices.available.size();
		VideoErrors_init(&videoThumbnailInfo->errors);
		Video videoInfo(videoThumbnailInfo->filename, &videoThumbnailInfo->errors, devices, deviceIndex);
		if (!videoInfo) {
			if (videoInfo.hasDeviceError()) {
				// Device error when loading video: move to next loop step.
				continue;
			}
			// Fatal error (no device error). Stop.
			return false;
		}
		// Video loaded.
		// Generate thumbnail.
		if (!videoInfo.generateThumbnail(thFolder, videoThumbnailInfo->thumbnailName)) {
			if (videoInfo.hasDeviceError()) {
				// Device error when generating thumbnail: move to next loop step.
				continue;
			}
			// Fatal error (no device error). Stop.
			return false;
		};
		// Update HW device index used.
		devices.indexUsed = indexToUse;
		return true;
	};
	// Device error for all devices. Don't use devices.
	videoRaptorError(&videoThumbnailInfo->errors, WARNING_NO_DEVICE_CODEC);
	devices.indexUsed = 0;
	Video videoInfo(videoThumbnailInfo->filename, &videoThumbnailInfo->errors, devices, devices.indexUsed);
	if (!videoInfo)
		return false;
	return videoInfo.generateThumbnail(thFolder, videoThumbnailInfo->thumbnailName);
}

inline bool getVideoDetails(HWDevices& devices, VideoDetails* videoDetails) {
	for (size_t i = 0; i < devices.available.size(); ++i) {
		size_t indexToUse = (devices.indexUsed + i) % devices.available.size();
		VideoErrors_init(&videoDetails->errors);
		Video videoInfo(videoDetails->filename, &videoDetails->errors, devices, indexToUse);
		if (!videoInfo) {
			if (videoInfo.hasDeviceError()) {
				// Device error when loading video: move to next loop step.
				continue;
			}
			// Fatal error (no device error). Stop.
			return false;
		}
		// Video loaded.
		videoInfo.extractInfo(videoDetails);
		// Update HW device index used.
		devices.indexUsed = indexToUse;
		// Work done.
		return true;
	}
	// Device error for all devices. Don't use devices.
	videoRaptorError(videoDetails, WARNING_NO_DEVICE_CODEC);
	devices.indexUsed = 0;
	VideoErrors_init(&videoDetails->errors);
	Video videoInfo(videoDetails->filename, &videoDetails->errors, devices, devices.indexUsed);
	if (!videoInfo)
		return false;
	videoInfo.extractInfo(videoDetails);
	return true;
}

int videoRaptorThumbnails(int length, VideoThumbnailInfo** pVideoThumbnailInfo, const char* thumbFolder) {
	if (length <= 0 || !pVideoThumbnailInfo || !thumbFolder)
		return ERROR_NO_BATCH_GIVEN;
	HWDevices* devices = nullptr;
	int initStatus = videoRaptorInit(&devices);
	if (initStatus != ERROR_CODE_OK)
		return initStatus;
	for (int i = 0; i < length; ++i) {
		VideoThumbnailInfo* videoThumbnailInfo = pVideoThumbnailInfo[i];
		if (!videoThumbnailInfo || !videoThumbnailInfo->filename || !videoThumbnailInfo->thumbnailName)
			continue;
		getVideoThumbnails(*devices, videoThumbnailInfo, thumbFolder);
	}
	return ERROR_CODE_OK;
}

int videoRaptorDetails(int length, VideoDetails** pVideoDetails) {
	// Returns an error code (ERROR_CODE_OK if no error).
	if (length <= 0 || !pVideoDetails)
		return ERROR_NO_BATCH_GIVEN;
	HWDevices* devices = nullptr;
	int initStatus = videoRaptorInit(&devices);
	if (initStatus != ERROR_CODE_OK)
		return initStatus;
	for (int i = 0; i < length; ++i) {
		VideoDetails* videoDetails = pVideoDetails[i];
		if (!videoDetails || !videoDetails->filename)
			continue;
		getVideoDetails(*devices, videoDetails);
	}
	return ERROR_CODE_OK;
}

