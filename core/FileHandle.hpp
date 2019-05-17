//
// Created by notoraptor on 28/07/2018.
//

#ifndef VIDEORAPTOR_FILEHANDLE_HPP
#define VIDEORAPTOR_FILEHANDLE_HPP

#include <cstdio>
#include <vector>

struct FileHandle {
	const char* filename;
	std::vector<wchar_t> unicodeFilename;
	FILE* file;
	explicit FileHandle(const char* handledFileName = nullptr): filename(handledFileName), unicodeFilename(), file(nullptr) {}
	~FileHandle() {
		if (file)
			fclose(file);
	}
};

#endif //VIDEORAPTOR_FILEHANDLE_HPP
