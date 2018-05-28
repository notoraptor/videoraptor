//
// Created by notoraptor on 27/05/2018.
//

#include <core/common.hpp>
#include "videoRaptorOnce.hpp"

bool videoRaptorOnce(const char* videoFilename, const char* thumbnailFolder, const char* thumbnailName, void* output) {
	HWDevices* devices;
	std::basic_ostream<char>& out = output ? ((Output*)output)->getOstream() : (std::basic_ostream<char>&)std::cout;
	if (!(devices = initVideoRaptor(out)))
		return false;
	return run(*devices, out, videoFilename, thumbnailFolder, thumbnailName);
}
