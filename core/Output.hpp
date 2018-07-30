//
// Created by notoraptor on 24/06/2018.
//

#ifndef VIDEORAPTOR_OUTPUT_HPP
#define VIDEORAPTOR_OUTPUT_HPP

#include <sstream>

struct Output {
	std::ostringstream oss;
	char* str;
	explicit Output(): oss(), str(nullptr) {}
	~Output() { delete[] str; }
	std::basic_ostream<char>& getStream() { return oss; }
	char* flush();
};

#endif //VIDEORAPTOR_OUTPUT_HPP
