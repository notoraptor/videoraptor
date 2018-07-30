//
// Created by notoraptor on 28/07/2018.
//

#ifndef VIDEORAPTOR_FILEHANDLE_HPP
#define VIDEORAPTOR_FILEHANDLE_HPP

#include <cstdio>

struct FileHandle {
	const char* filename;
	std::vector<wchar_t> unicodeFilename;
	FILE* file;
	FileHandle(): filename(nullptr), unicodeFilename(), file(nullptr) {}
};

#endif //VIDEORAPTOR_FILEHANDLE_HPP
