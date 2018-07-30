//
// Created by notoraptor on 24/06/2018.
//

#ifndef VIDEORAPTOR_APP_ERROR_HPP
#define VIDEORAPTOR_APP_ERROR_HPP

#include <sstream>

class AppError {
	std::ostringstream oss;
	std::basic_ostream<char>& out;

	void toHexadecimal(const char* str) {
		static const char* const digits = "0123456789ABCDEF";
		size_t stringLength = strlen(str);
		std::string output(2 * stringLength, ' ');
		for (size_t i = 0; i < stringLength; ++i) {
			unsigned char c = str[i];
			output[2 * i] = digits[c >> 4];
			output[2 * i + 1] = digits[c & (((unsigned int) 1 << 4) - 1)];
		}
		oss << output;
	}

public:

	explicit AppError(std::basic_ostream<char>& output, const char* filename = nullptr, bool isError = true):
			oss(), out(output) {
		if (filename) {
			oss << "#VIDEO_" << (isError ? "ERROR" : "WARNING") << "[";
			toHexadecimal(filename);
			oss << "]";
		} else {
			oss << (isError ? "#ERROR " : "#WARNING ");
		}
	}

	AppError& write(const char* message) {
		oss << message;
		return *this;
	}

	AppError& write(const std::string& message) {
		oss << message;
		return *this;
	}

	operator bool() const { return false; }

	~AppError() { out << oss.str() << std::endl; }
};

struct AppWarning: public AppError {
	explicit AppWarning(std::basic_ostream<char>& output, const char* filename = nullptr): AppError(output, filename, false) {}
};

#endif //VIDEORAPTOR_APP_ERROR_HPP
