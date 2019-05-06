//
// Created by notoraptor on 27/05/2018.
//

#include <string>
#include <fstream>
#include <iostream>
#include <core/Video.hpp>
#include <core/errorCodes.hpp>
#include "videoRaptorBatch.hpp"

typedef bool (*VideoWorkerFunction)(Video* video, void* context);

bool videoWorkerForInfo(Video* video, void* context) {
	video->extractInfo((VideoInfo*)context);
	return true;
}

bool videoWorkerForThumbnail(Video* video, void* context) {
	return video->generateThumbnail((VideoThumbnail*)context);
}

bool workOnVideo(HWDevices &devices, const char* videoFilename, VideoReport* videoReport, void* videoContext, VideoWorkerFunction videoWorkerFunction) {
	for (size_t i = 0; i < devices.available.size(); ++i) {
		size_t indexToUse = (devices.indexUsed + i) % devices.available.size();
		VideoReport_init(videoReport);
		Video video(videoFilename, videoReport, devices, indexToUse);
		if (VideoReport_hasError(videoReport)) {
			if (VideoReport_hasDeviceError(videoReport)) {
				// Device error when loading video: move to next loop step.
				continue;
			}
			// Fatal error (no device error). Stop.
			return false;
		}
		// Video loaded. Do work.
		if (!videoWorkerFunction(&video, videoContext)) {
			if (VideoReport_hasDeviceError(videoReport)) {
				// Device error when working: move to next loop step.
				continue;
			}
			// Fatal error (no device error). Stop.
			return false;
		}
		// Update HW device index used.
		devices.indexUsed = indexToUse;
		return true;
	};
	// Device error for all devices. Don't use devices. Set index to invalid value.
	devices.indexUsed = devices.available.size();
	VideoReport_init(videoReport);
	Video video(videoFilename, videoReport, devices, devices.indexUsed);
	if (VideoReport_hasError(videoReport))
		return false;
	return videoWorkerFunction(&video, videoContext);
}

int videoRaptorThumbnails(int length, VideoThumbnail** pVideoThumbnail) {
	if (length <= 0 || !pVideoThumbnail)
		return 0;
	HWDevices* devices = getHardwareDevices();
	int countLoaded = 0;
	for (int i = 0; i < length; ++i) {
		VideoThumbnail* videoThumbnail = pVideoThumbnail[i];
		if (videoThumbnail
			&& videoThumbnail->filename
			&& videoThumbnail->thumbnailFolder
			&& videoThumbnail->thumbnailName
			&& workOnVideo(*devices, videoThumbnail->filename, &videoThumbnail->report, videoThumbnail, videoWorkerForThumbnail))
			++countLoaded;
	}
	return countLoaded;
}

int videoRaptorDetails(int length, VideoInfo** pVideoInfo) {
	if (length <= 0 || !pVideoInfo)
		return 0;
	HWDevices* devices = getHardwareDevices();
	int countLoaded = 0;
	for (int i = 0; i < length; ++i) {
		VideoInfo* videoDetails = pVideoInfo[i];
		if (videoDetails
			&& videoDetails->filename
			&& workOnVideo(*devices, videoDetails->filename, &videoDetails->report, videoDetails, videoWorkerForInfo))
			++countLoaded;
	}
	return countLoaded;
}

