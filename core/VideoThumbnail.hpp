//
// Created by notoraptor on 15/04/2019.
//

#ifndef VIDEORAPTOR_VIDEOTHUMBNAIL_HPP
#define VIDEORAPTOR_VIDEOTHUMBNAIL_HPP

#include "VideoReport.hpp"

struct VideoThumbnail {
	// Inputs:
	const char* filename;
	const char* thumbnailFolder;
	const char* thumbnailName;
	// Outputs:
	VideoReport report;
	// Use VideoReport_isDone(&videoThumbnail.report) to check if thumbnail was correctly generated.
};

extern "C" {
	void VideoThumbnail_init(VideoThumbnail* videoThumbnail, const char* filename, const char* thumbnailFolder, const char* thumbnailName);
}

#endif //VIDEORAPTOR_VIDEOTHUMBNAIL_HPP
