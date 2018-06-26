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
	char* flushLogger();
	bool videoRaptorBatch(int length, const char** fileNames,
						  const char** thumbNames, const char* thumbFolder, void* output);
	bool videoRaptorDetails(int length, const char** fileNames, VideoDetails** pVideoDetails, void* output);
	bool videoRaptorTxtFile(const char* filename, void* output);
};


#endif //VIDEORAPTOR_VIDEORAPTORBATCH_HPP
