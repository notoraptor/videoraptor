//
// Created by notoraptor on 27/05/2018.
//

#include <string>
#include <fstream>
#include <iostream>
#include <core/initVideoRaptor.hpp>
#include <core/Output.hpp>
#include <core/Video.hpp>
#include "videoRaptorBatch.hpp"

void* createOutput() {
	return new Output();
}

const char* outputToString(void* out) {
	return ((Output*)out)->flush();
}

void deleteOutput(void* out) {
	delete (Output*)out;
}

inline bool getVideoThumbnails(HWDevices &devices, std::basic_ostream<char> &out, const char *filename,
							   const char *thFolder, const char *thName) {
	size_t deviceIndex = devices.indexUsed;
	while (deviceIndex < devices.available.size()) {
		Video videoInfo(out);
		if (!videoInfo.load(filename, devices, deviceIndex)) {
			if (videoInfo.hasDeviceError()) {
				out << "#WARNING Device error when loading video." << std::endl;
				++deviceIndex;
				continue;
			}
			return false;
		}
		// Generate thumbnail.
		if (!videoInfo.generateThumbnail(thFolder, thName)) {
			if (videoInfo.hasDeviceError()) {
				out << "#WARNING Device error when generating thumbnail." << std::endl;
				++deviceIndex;
				continue;
			}
			return false;
		};
		devices.indexUsed = deviceIndex;
		return true;
	};
	AppWarning(out, filename).write("Opened without device codec.");
	Video videoInfo(out);
	if (!videoInfo.load(filename, devices, deviceIndex))
		return false;
	return videoInfo.generateThumbnail(thFolder, thName);
}

inline bool getVideoDetails(HWDevices& devices, std::basic_ostream<char>& out, const char* filename,
							VideoDetails* videoDetails) {
	size_t deviceIndex = devices.indexUsed;
	while (deviceIndex < devices.available.size()) {
		Video videoInfo(out);
		if (!videoInfo.load(filename, devices, deviceIndex)) {
			if (videoInfo.hasDeviceError()) {
				out << "#WARNING Device error when loading video." << std::endl;
				++deviceIndex;
				continue;
			}
			return false;
		}
		videoInfo.extractInfo(videoDetails);
		devices.indexUsed = deviceIndex;
		return true;
	};
	AppWarning(out, filename).write("Opened without device codec.");
	devices.indexUsed = 0;
	Video videoInfo(out);
	if (!videoInfo.load(filename, devices, deviceIndex))
		return false;
	videoInfo.extractInfo(videoDetails);
	return true;
}

int videoRaptorThumbnails(int length, const char** fileNames, const char** thumbNames, const char* thumbFolder, void* output) {
	int countGood = 0;
	std::basic_ostream<char>& out = output ? ((Output*) output)->getStream() : (std::basic_ostream<char>&)std::cout;
	HWDevices* devices = nullptr;
	if (!(devices = initVideoRaptor(out)))
		return false;
	if (length <= 0 || !fileNames || !thumbNames || !thumbFolder)
		return AppError(out).write("No batch given.");
	for (int i = 0; i < length; ++i) {
		const char* videoFilename = fileNames[i];
		const char* thumbnailName = thumbNames[i];
		if (!videoFilename || !thumbnailName)
			continue;
		countGood += getVideoThumbnails(*devices, out, videoFilename, thumbFolder, thumbnailName);
	}
	return countGood;
}

int videoRaptorDetails(int length, const char** fileNames, VideoDetails** pVideoDetails, void* output) {
	int countGood = 0;
	std::basic_ostream<char>& out = output ? ((Output*) output)->getStream() : (std::basic_ostream<char>&)std::cout;
	HWDevices* devices = nullptr;
	if (!(devices = initVideoRaptor(out)))
		return false;
	if (length <= 0 || !fileNames || !pVideoDetails)
		return AppError(out).write("No batch given.");
	for (int i = 0; i < length; ++i) {
		const char* videoFilename = fileNames[i];
		VideoDetails* videoDetails = pVideoDetails[i];
		if (!videoFilename || !videoDetails)
			continue;
		countGood += getVideoDetails(*devices, out, videoFilename, videoDetails);
	}
	return countGood;
}

