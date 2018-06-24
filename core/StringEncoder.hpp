//
// Created by notoraptor on 24/06/2018.
//

#ifndef VIDEORAPTOR_STRINGENCODER_HPP
#define VIDEORAPTOR_STRINGENCODER_HPP

#include <cstring>
#include <ostream>
#include <iostream>

extern const char* const digits;

enum class EncoderType { HEX, JSON };

struct StringEncoder {
	const char* str;
	EncoderType type;

	explicit StringEncoder(const char* s, EncoderType encoderType): str(s), type(encoderType) {}

	char* encode() {
		size_t stringLength = strlen(str);
		char* output = new char[2 * stringLength + 3];
		size_t countWritten = 1;
		output[0] = '"';
		switch (type) {
			case EncoderType::HEX:
				for (size_t i = 0; i < stringLength; ++i) {
					unsigned char c = str[i];
					output[countWritten + 2 * i] = digits[c >> 4];
					output[countWritten + 2 * i + 1] = digits[c & (((unsigned int) 1 << 4) - 1)];
				}
				countWritten += 2 * stringLength;
				break;
			case EncoderType::JSON:
				for (size_t i = 0; i < stringLength; ++i) {
					if (str[i] == '"') {
						output[countWritten] = '\\';
						output[countWritten + 1] = '"';
						countWritten += 2;
					} else if (str[i] == '\\') {
						output[countWritten] = '\\';
						output[countWritten + 1] = '\\';
						countWritten += 2;
					} else {
						output[countWritten] = str[i];
						++countWritten;
					}
				}
				break;
		}
		output[countWritten] = '"';
		output[countWritten + 1] = '\0';
		return output;
	}
};

inline std::ostream& operator<<(std::ostream& o, const StringEncoder& stringEncoder) {
	const char* const digits = "0123456789ABCDEF";
	o << '"';
	const char* str = stringEncoder.str;
	switch (stringEncoder.type) {
		case EncoderType::HEX:
			while (*str) {
				unsigned char c = *str;
				o << digits[c >> 4] << digits[c & (((unsigned int) 1 << 4) - 1)];
				++str;
			}
			break;
		case EncoderType::JSON:
			while (*str) {
				if (*str == '"')
					o << "\\\"";
				else if (*str == '\\')
					o << "\\\\";
				else
					o << *str;
				++str;
			}
			break;
	}
	o << '"';
	return o;
}

#endif //VIDEORAPTOR_STRINGENCODER_HPP
