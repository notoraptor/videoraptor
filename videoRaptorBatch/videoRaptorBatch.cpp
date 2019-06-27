//
// Created by notoraptor on 27/05/2018.
//

#include <cmath>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <core/Video.hpp>
#include <core/errorCodes.hpp>
#include "videoRaptorBatch.hpp"

typedef bool (* VideoWorkerFunction)(Video* video, void* context);

bool videoWorkerForInfo(Video* video, void* context) {
	video->extractInfo((VideoInfo*) context);
	return true;
}

bool videoWorkerForThumbnail(Video* video, void* context) {
	return video->generateThumbnail((VideoThumbnail*) context);
}

bool workOnVideo(HWDevices& devices, const char* videoFilename, VideoReport* videoReport, void* videoContext,
				 VideoWorkerFunction videoWorkerFunction) {
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
			&& workOnVideo(*devices, videoThumbnail->filename, &videoThumbnail->report, videoThumbnail,
						   videoWorkerForThumbnail))
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


int alignmentScore(std::vector<int>& matrix, const int* a, const int* b, int columns, int matchScore, int diffScore, int gapScore) {
	int sideLength = columns + 1;
	int matrixSize = sideLength * sideLength;
	for (int i = 0; i < sideLength; ++i) {
		matrix[i] = i * gapScore;
	}
	for (int j = 1; j < sideLength; ++j) {
		matrix[j * sideLength] = j * gapScore;
	}
	for (int i = 1; i < sideLength; ++i) {
		for (int j = 1; j < sideLength; ++j) {
			matrix[i * sideLength + j] = std::max(
				matrix[(i - 1) * sideLength + (j - 1)] + (a[i - 1] == b[j - 1] ? matchScore : diffScore),
				std::max(
					matrix[(i - 1) * sideLength + j] + gapScore,
					matrix[i * sideLength + (j - 1)] + gapScore
				)
			);
		}
	}
	return matrix[matrixSize - 1];
}

int batchAlignmentScore(const int** A, const int** B, int rows, int columns, int matchScore, int diffScore, int gapScore) {
	int totalScore = 0;
	int sideLength = columns + 1;
	int matrixSize = sideLength * sideLength;
	std::vector<int> matrix(matrixSize, 0);
	for (int i = 0; i < rows; ++i)
		totalScore += alignmentScore(matrix, A[i], B[i], columns, matchScore, diffScore, gapScore);
	return totalScore;
};

double alignmentScoreByDiff(std::vector<double>& matrix, const int* a, const int* b, int columns, double interval, int gapScore) {
	int sideLength = columns + 1;
	int matrixSize = sideLength * sideLength;
	for (int i = 0; i < sideLength; ++i) {
		matrix[i] = i * gapScore;
	}
	for (int j = 1; j < sideLength; ++j) {
		matrix[j * sideLength] = j * gapScore;
	}
	for (int i = 1; i < sideLength; ++i) {
		for (int j = 1; j < sideLength; ++j) {
			matrix[i * sideLength + j] = std::max(
					matrix[(i - 1) * sideLength + (j - 1)] + 2 * ((interval - abs(a[i - 1] - b[j - 1]))/interval) - 1,
					std::max(
							matrix[(i - 1) * sideLength + j] + gapScore,
							matrix[i * sideLength + (j - 1)] + gapScore
					)
			);
		}
	}
	return matrix[matrixSize - 1];
}

double batchAlignmentScoreByDiff(const int* A, const int* B, int rows, int columns, int minVal, int maxVal, int gapScore) {
	double totalScore = 0;
	int sideLength = columns + 1;
	int matrixSize = sideLength * sideLength;
	std::vector<double> matrix(matrixSize, 0);
	double interval = maxVal - minVal;
	for (int i = 0; i < rows; ++i)
		totalScore += alignmentScoreByDiff(matrix, A + i * columns, B + i * columns, columns, interval, gapScore);
	return totalScore;
}
