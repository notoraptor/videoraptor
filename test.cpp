//
// Created by notoraptor on 27/07/2018.
//

#include <sstream>
#include <core/VideoRaptorInfo.hpp>
#include <videoRaptorBatch/videoRaptorBatch.hpp>
#include <core/ErrorReader.hpp>
#include <alignment/alignment.hpp>

void printDetails(VideoInfo* videoDetails) {
	std::cout << "BEGIN DETAILS" << std::endl;
	if (videoDetails->filename)
		std::cout << "\tfilename          : " << videoDetails->filename << std::endl;
	if (videoDetails->title)
		std::cout << "\ttitle             : " << videoDetails->title << std::endl;
	if (videoDetails->container_format)
		std::cout << "\tcontainer_format  : " << videoDetails->container_format << std::endl;
	if (videoDetails->audio_codec)
		std::cout << "\taudio_codec       : " << videoDetails->audio_codec << std::endl;
	if (videoDetails->video_codec)
		std::cout << "\tvideo_codec       : " << videoDetails->video_codec << std::endl;
	std::cout << "\twidth             : " << videoDetails->width << std::endl;
	std::cout << "\theight            : " << videoDetails->height << std::endl;
	std::cout << "\tframe_rate_num    : " << videoDetails->frame_rate_num << std::endl;
	std::cout << "\tframe_rate_den    : " << videoDetails->frame_rate_den << std::endl;
	std::cout << "\tduration          : " << videoDetails->duration << std::endl;
	std::cout << "\tduration_time_base: " << videoDetails->duration_time_base << std::endl;
	std::cout << "\tsize              : " << videoDetails->size << std::endl;
	std::cout << "\tsample_rate       : " << videoDetails->sample_rate << std::endl;
	std::cout << "\taudio_bit_rate    : " << videoDetails->audio_bit_rate << std::endl;
	std::cout << "END DETAILS" << std::endl;
}

bool testDetails(const char* filename) {
	bool returnValue = false;
	VideoInfo videoInfo;
	VideoInfo_init(&videoInfo, filename);
	VideoInfo* pVideoDetails = &videoInfo;
	videoRaptorDetails(1, &pVideoDetails);
	if (VideoReport_isDone(&videoInfo.report)) {
		printDetails(&videoInfo);
		returnValue = true;
	} else {
		std::cout << "No details." << std::endl;
	}
	if (VideoReport_hasError(&videoInfo.report)) {
		std::cout << "Video details: error(s) occurred (" << videoInfo.report.errors << ")." << std::endl;
		VideoReport_print(&videoInfo.report);
	}
	VideoInfo_clear(&videoInfo);
	return returnValue;
}

bool testThumbnail(const char* filename, const char* thumbName) {
	bool returnValue = false;
	VideoThumbnail videoThumbnail;
	VideoThumbnail_init(&videoThumbnail, filename, ".", thumbName);
	VideoThumbnail* pVideoThumbnailInfo = &videoThumbnail;
	videoRaptorThumbnails(1, &pVideoThumbnailInfo);
	if (VideoReport_isDone(&videoThumbnail.report)) {
		std::cout << "Thumbnail created: " << thumbName << ".png" << std::endl;
		returnValue = true;
	} else {
		std::cout << "No thumbnail." << std::endl;
	}
	if (VideoReport_hasError(&videoThumbnail.report)) {
		std::cout << "Video thumbnails: error(s) occurred (" << videoThumbnail.report.errors << ")." << std::endl;
		VideoReport_print(&videoThumbnail.report);
	}
	return returnValue;
}

void test(const char* filename, const char* thumbName) {
	if (testDetails(filename))
		testThumbnail(filename, thumbName);
}

void testErrorPrinting() {
	std::cout << "Testing errors printing ..." << std::endl;
	unsigned int errors = ERROR_OPEN_FILE | ERROR_CODE_000000032 | ERROR_CONVERT_CODEC_PARAMS | ERROR_PNG_CODEC;
	ErrorReader errorReader;
	ErrorReader_init(&errorReader, errors);
	while (const char* errorString = ErrorReader_next(&errorReader)) {
		std::cout << errorString << std::endl;
	}
	std::cout << "... Finished testing." << std::endl << std::endl;
}

void printVideoRaptorInfo() {
	// Print info about available hardware acceleration.
	VideoRaptorInfo videoRaptorInfo;
	VideoRaptorInfo_init(&videoRaptorInfo);
	std::cout << videoRaptorInfo.hardwareDevicesCount << " hardware device(s)";
	if (videoRaptorInfo.hardwareDevicesCount)
		std::cout << ": " << videoRaptorInfo.hardwareDevicesNames;
	std::cout << '.' << std::endl << std::endl;
	VideoRaptorInfo_clear(&videoRaptorInfo);
}

int main() {
	return EXIT_SUCCESS;
}