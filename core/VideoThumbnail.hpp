//
// Created by notoraptor on 15/04/2019.
//

#ifndef VIDEORAPTOR_VIDEOTHUMBNAIL_HPP
#define VIDEORAPTOR_VIDEOTHUMBNAIL_HPP

#include "VideoLog.hpp"

struct VideoThumbnail {
	// Inputs:
	const char* filename;
	const char* thumbnailFolder;
	const char* thumbnailName;
	// Outputs:
	VideoLog errors;
};

extern "C" {
	void VideoThumbnail_init(VideoThumbnail* videoThumbnail, const char* filename, const char* thumbnailFolder, const char* thumbnailName);
}

#endif //VIDEORAPTOR_VIDEOTHUMBNAIL_HPP
