//
// Created by notoraptor on 27/07/2018.
//

#include <sstream>
#include <core/VideoRaptorInfo.hpp>
#include <videoRaptorBatch/videoRaptorBatch.hpp>

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
	std::cout << "\tsample_rate       : " << videoDetails->sample_rate << std::endl;
	std::cout << "\tduration          : " << videoDetails->duration << std::endl;
	std::cout << "\tduration_time_base: " << videoDetails->duration_time_base << std::endl;
	std::cout << "\tsize              : " << videoDetails->size << std::endl;
	std::cout << "\tbit_rate          : " << videoDetails->bit_rate << std::endl;
	std::cout << "END DETAILS" << std::endl;
}

bool testDetails(const char* filename) {
	VideoInfo videoDetails;
	VideoInfo_init(&videoDetails, filename);
	VideoInfo* pVideoDetails = &videoDetails;
	videoRaptorDetails(1, &pVideoDetails);
	if (VideoReport_hasError(&videoDetails.report)) {
		std::cout << "Video details: error(s) occurred (" << videoDetails.report.errors << ")." << std::endl;
		VideoReport_print(&videoDetails.report);
		return false;
	}
	if (!VideoReport_isDone(&videoDetails.report)) {
		std::cout << "No details." << std::endl;
		return false;
	}
	printDetails(&videoDetails);
	VideoInfo_clear(&videoDetails);
	return true;
}

bool testThumbnail(const char* filename, const char* thumbName) {
	VideoThumbnail videoThumbnail;
	VideoThumbnail_init(&videoThumbnail, filename, ".", thumbName);
	VideoThumbnail* pVideoThumbnailInfo = &videoThumbnail;
	videoRaptorThumbnails(1, &pVideoThumbnailInfo);
	if (VideoReport_hasError(&videoThumbnail.report)) {
		std::cout << "Video thumbnails: error(s) occurred (" << videoThumbnail.report.errors << ")." << std::endl;
		VideoReport_print(&videoThumbnail.report);
		return false;
	}
	if (!VideoReport_isDone(&videoThumbnail.report)) {
		std::cout << "No thumbnail." << std::endl;
		return false;
	}
	std::cout << "Thumbnail created: " << thumbName << ".png" << std::endl;
	return true;
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

int main(int lenArgs, char* args[]) {
	// Test error printing.
	testErrorPrinting();

	// Print some info.
	printVideoRaptorInfo();

	// Parse arguments.
	if (lenArgs < 2) {
		std::cerr << "No files provided." << std::endl;
		return EXIT_FAILURE;
	}
	for (int i = 1; i < lenArgs; ++i) {
		std::ostringstream oss;
		oss << i;
		test(args[i], oss.str().c_str());
	}
	return EXIT_SUCCESS;
}