//
// Created by notoraptor on 27/07/2018.
//

#include <videoRaptorBatch/videoRaptorBatch.hpp>
#include <sstream>
#include <core/videoRaptorInit.hpp>
#include <core/errorCodes.hpp>

inline void printDetails(VideoDetails* videoDetails) {
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

void test(const char* filename, const char* thumbName = nullptr) {
	VideoDetails videoDetails;
	VideoThumbnailInfo videoThumbnailInfo;
	videoDetails.filename = filename;
	videoThumbnailInfo.filename = filename;
	videoThumbnailInfo.thumbnailName = thumbName;
	VideoDetails* pVideoDetails = &videoDetails;
	VideoThumbnailInfo* pVideoThumbnailInfo = &videoThumbnailInfo;
	if (videoRaptorDetails(1, &pVideoDetails)) {
		printDetails(pVideoDetails);
		if (thumbName && videoRaptorThumbnails(1, &pVideoThumbnailInfo, "."))
			std::cout << "Thumbnail created: " << thumbName << ".png" << std::endl;
	}
}

void printHWDT() {
	HWDevices* d;
	int e = videoRaptorInit(&d);
	if (e == ERROR_CODE_OK) {
		std::cout << d->countDeviceTypes() << std::endl;
		const char* sep = ", ";
		size_t t = d->getStringRepresentationLength(sep);
		if (t) {
			std::vector<char> s(t, ' ');
			d->getStringRepresentation(s.data(), sep);
			std::cout << s.data() << std::endl;
		}
	}
}

char* hwDeviceNames() {
	char* output = nullptr;
	HWDevices* hwDevices;
	int e = videoRaptorInit(&hwDevices);
	if (e == ERROR_CODE_OK) {
		const char* sep = ", ";
		size_t stringRepresentationLength = hwDevices->getStringRepresentationLength(sep);
		if (stringRepresentationLength) {
			output = (char*) malloc(stringRepresentationLength);
			hwDevices->getStringRepresentation(output, sep);
		}
	}
	return output;
}

int main(int lenArgs, char* args[]) {
	printHWDT();
	if (lenArgs > 1) {
		for (int i = 1; i < lenArgs; ++i) {
			std::ostringstream oss;
			oss << i;
			test(args[i], oss.str().c_str());
		}
		return EXIT_SUCCESS;
	}
	std::cerr << "No files provided." << std::endl;
	return EXIT_FAILURE;
}