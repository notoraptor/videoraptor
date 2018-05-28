//
// Created by notoraptor on 27/05/2018.
//

#include <string>
#include <core/common.hpp>
#include <fstream>
#include <iostream>
#include "videoRaptorBatch.hpp"

inline void stripString(std::string& s) {
	size_t stripLeft = std::string::npos;
	for (size_t i = 0; i < s.length(); ++i) {
		if (!std::isspace(s[i])) {
			stripLeft = i;
			break;
		}
	}
	if (stripLeft == std::string::npos) {
		s.clear();
	} else {
		s.erase(0, stripLeft);
		for (size_t i = s.length(); i > 0; --i) {
			if (!std::isspace(s[i - 1])) {
				if (i != s.length())
					s.erase(i);
				break;
			}
		}
	}
}

void* createOutput() {
	return new Output();
}
const char* outputToString(void* out) {
	return ((Output*)out)->flush();
}
void deleteOutput(void* out) {
	delete (Output*)out;
}

bool videoRaptorBatch(int length, const char** fileNames, const char** thumbNames, const char* thumbFolder, void* output) {
	std::basic_ostream<char>& out = output ? ((Output*)output)->getOstream() : (std::basic_ostream<char>&)std::cout;
	HWDevices* devices = nullptr;
	if (!(devices = initVideoRaptor(out)))
		return false;
	if (length <= 0 || !fileNames)
		return AppError(out).write("No batch given.");
	for (int i = 0; i < length; ++i) {
		const char* videoFilename = fileNames[i];
		const char* thumbnailName = thumbFolder ? thumbNames[i] : nullptr;
		if (!videoFilename)
			continue;
		if (run(*devices, out, videoFilename, thumbnailName ? thumbFolder : nullptr, thumbnailName)) {
			if ((i + 1) % 25 == 0)
				out << "#LOADED " << (i + 1) << std::endl;
		} else {
			out << "#IGNORED " << videoFilename << std::endl;
		}
	}
	return true;
}

bool videoRaptorTxtFile(const char* filename, void* output) {
	HWDevices* devices = nullptr;
	std::ifstream textFile(filename);
	std::string line;
	size_t count = 0;
	std::basic_ostream<char>& out = output ? ((Output*)output)->getOstream() : (std::basic_ostream<char>&)std::cout;
	if (!(devices = initVideoRaptor(out)))
		return false;
	if (!textFile)
		return AppError(out).write("Invalid file name ").write(filename);
	while (std::getline(textFile, line)) {
		stripString(line);
		if (!line.empty() && line[0] != '#') {
			size_t posTab1 = line.find('\t', 0);
			size_t posTab2 = (posTab1 != std::string::npos) ? line.find('\t', posTab1 + 1) : std::string::npos;
			const char* videoFilename = line.c_str();
			const char* thumbnailFolder = nullptr;
			const char* thumbnailName = nullptr;
			if (posTab1 != std::string::npos) {
				if (posTab2 == std::string::npos) {
					out << "#ERROR line " << line << std::endl;
					out << "#ERROR Bad syntax (expected 3 columns separated by a tab)." << std::endl;
					videoFilename = nullptr;
				} else {
					line[posTab1] = line[posTab2] = '\0';
					thumbnailFolder = line.c_str() + posTab1 + 1;
					thumbnailName = line.c_str() + posTab2 + 1;
				}
			}
			if (videoFilename && run(*devices, out, videoFilename, thumbnailFolder, thumbnailName)) {
				if ((++count) % 25 == 0)
					out << "#LOADED " << count << std::endl;
			} else
				out << "#IGNORED " << line << std::endl;
		}
	}
	out << "#FINISHED " << count << std::endl;
	return true;
}
