//
// Created by notoraptor on 27/07/2018.
//

#include <videoRaptorBatch/videoRaptorBatch.hpp>
#include <sstream>

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
	VideoDetails* pVideoDetails = &videoDetails;
	if (videoRaptorDetails(1, &filename, &pVideoDetails, nullptr)) {
		printDetails(pVideoDetails);
		if (thumbName && videoRaptorThumbnails(1, &filename, &thumbName, ".", nullptr))
			std::cout << "Thumbnail created: " << thumbName << ".png" << std::endl;
	}
}

int main(int lenArgs, char* args[]) {
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