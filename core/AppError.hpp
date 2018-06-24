//
// Created by notoraptor on 24/06/2018.
//

#ifndef VIDEORAPTOR_APPERROR_HPP
#define VIDEORAPTOR_APPERROR_HPP

#include <sstream>

class AppError {
	std::ostringstream oss;
	std::basic_ostream<char>& out;
public:
	explicit AppError(std::basic_ostream<char>& output, const char* filename = nullptr): oss(), out(output) {
		if (filename)
			oss << "#VIDEO_ERROR[" << filename << "]";
		else
			oss << "#ERROR ";
	}

	AppError& write(const char* message) {
		oss << message;
		return *this;
	}

	operator bool() const { return false; }

	~AppError() { out << oss.str() << std::endl; }
};

#endif //VIDEORAPTOR_APPERROR_HPP
