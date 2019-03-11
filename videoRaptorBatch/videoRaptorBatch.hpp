//
// Created by notoraptor on 27/05/2018.
//

#ifndef VIDEORAPTOR_VIDEORAPTORBATCH_HPP
#define VIDEORAPTOR_VIDEORAPTORBATCH_HPP

#include <core/VideoDetails.hpp>

extern "C" {
	int videoRaptorDetails(int length, VideoDetails** pVideoDetails);
	int videoRaptorThumbnails(int length, VideoThumbnailInfo** pVideoThumbnailInfo, const char* thumbFolder);
};


#endif //VIDEORAPTOR_VIDEORAPTORBATCH_HPP
