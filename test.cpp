//
// Created by notoraptor on 27/07/2018.
//

#include <videoRaptorBatch/videoRaptorBatch.hpp>
#include <sstream>

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