//
// Created by notoraptor on 13/03/2019.
//

#ifndef VIDEORAPTOR_VIDEORAPTORINFO_HPP
#define VIDEORAPTOR_VIDEORAPTORINFO_HPP

#include "utils.hpp"
#include "errorCodes.hpp"

struct VideoRaptorInfo {
	size_t hardwareDevicesCount;
	char* hardwareDevicesNames;
};

extern "C" {
	void VideoRaptorInfo_init(VideoRaptorInfo* videoRaptorInfo);
	void VideoRaptorInfo_clear(VideoRaptorInfo* videoRaptorInfo);
}

#endif //VIDEORAPTOR_VIDEORAPTORINFO_HPP
