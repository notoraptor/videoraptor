//
// Created by notoraptor on 27/05/2018.
//

#ifndef VIDEORAPTOR_VIDEORAPTORBATCH_HPP
#define VIDEORAPTOR_VIDEORAPTORBATCH_HPP

#include <core/VideoDetails.hpp>

extern "C" {
	void* createOutput();
	const char* outputToString(void*);
	void deleteOutput(void*);
	int videoRaptorDetails(int length, const char** fileNames, VideoDetails** pVideoDetails, void* output);
	int videoRaptorThumbnails(int length, const char** fileNames, const char** thumbNames, const char* thumbFolder, void* output);
};


#endif //VIDEORAPTOR_VIDEORAPTORBATCH_HPP
