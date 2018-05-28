//
// Created by notoraptor on 27/05/2018.
//

#ifndef VIDEORAPTOR_VIDEORAPTORBATCH_HPP
#define VIDEORAPTOR_VIDEORAPTORBATCH_HPP

extern "C" {
	void* createOutput();
	const char* outputToString(void*);
	void deleteOutput(void*);
	bool videoRaptorTxtFile(const char* filename, void* output);
	bool videoRaptorBatch(int length, const char** fileNames, const char** thumbNames, const char* thumbFolder, void* output);
};


#endif //VIDEORAPTOR_VIDEORAPTORBATCH_HPP
