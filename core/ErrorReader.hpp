//
// Created by notoraptor on 01/05/2019.
//

#ifndef VIDEORAPTOR_ERRORREADER_HPP
#define VIDEORAPTOR_ERRORREADER_HPP

struct ErrorReader {
	unsigned int errors;
	unsigned int position;
};

extern "C" {
	void ErrorReader_init(ErrorReader* errorReader, unsigned int errors);
	const char* ErrorReader_next(ErrorReader* errorReader);
}

#endif //VIDEORAPTOR_ERRORREADER_HPP
