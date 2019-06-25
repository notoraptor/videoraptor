//
// Created by notoraptor on 27/05/2018.
//

#ifndef VIDEORAPTOR_VIDEORAPTORBATCH_HPP
#define VIDEORAPTOR_VIDEORAPTORBATCH_HPP

#include <core/VideoInfo.hpp>
#include <core/VideoThumbnail.hpp>

extern "C" {
	int videoRaptorDetails(int length, VideoInfo** pVideoInfo);
	int videoRaptorThumbnails(int length, VideoThumbnail** pVideoThumbnail);
	int batchAlignmentScore(const int** A, const int** B, int rows, int columns, int matchScore, int diffScore, int gapScore);
	double batchAlignmentScoreByDiff(const int** A, const int** B, int rows, int columns, int minVal, int maxVal, int gapScore);
};


#endif //VIDEORAPTOR_VIDEORAPTORBATCH_HPP
