//
// Created by notoraptor on 24/06/2018.
//

#ifndef VIDEORAPTOR_OUTPUT_HPP
#define VIDEORAPTOR_OUTPUT_HPP

#include <sstream>

struct Output {
	std::ostringstream os;
	char* str;
	explicit Output(): os(), str(nullptr) {}
	std::basic_ostream<char>& getOstream() { return os; }
	char* flush() {
		delete[] str;
		std::string s = os.str();
		os.str(std::string());
		str = new char[s.length() + 1];
		memcpy(str, s.data(), s.length());
		str[s.length()] = '\0';
		return str;
	}
	~Output() { delete[] str; }
};

#endif //VIDEORAPTOR_OUTPUT_HPP
