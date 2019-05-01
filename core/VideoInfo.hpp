//
// Created by notoraptor on 23/06/2018.
//

#ifndef VIDEORAPTOR_VIDEO_DETAILS_HPP
#define VIDEORAPTOR_VIDEO_DETAILS_HPP

#include <cstdint>
#include "VideoReport.hpp"

struct VideoInfo {
	// Inputs:
	const char* filename;
	// Outputs:
	char* title;
	char* container_format;
	char* audio_codec;
	char* video_codec;
	int width;
	int height;
	int frame_rate_num;
	int frame_rate_den;
	int sample_rate;
	int64_t duration;
	int64_t duration_time_base;
	int64_t size;
	int64_t bit_rate;
	VideoReport report;
	// Use VideoReport_isDone(&videoInfo.report) to check if info was correctly collected.
};

extern "C" {
	void VideoInfo_init(VideoInfo* videoInfo, const char* filename);
	void VideoInfo_clear(VideoInfo* videoInfo);
	bool VideoInfo_error(VideoInfo* videoInfo, unsigned int errorCode, const char* errorDetail = nullptr);
}

#endif //VIDEORAPTOR_VIDEO_DETAILS_HPP
