//
// Created by notoraptor on 23/07/2018.
//

#ifndef VIDEORAPTORBATCH_HWDEVICES_HPP
#define VIDEORAPTORBATCH_HWDEVICES_HPP

extern "C" {
#include <libavutil/hwcontext.h>
};
#include <unordered_map>
#include <vector>
#include <ostream>

struct HWDevices {
	std::vector<AVHWDeviceType> available;
	std::unordered_map<AVHWDeviceType, AVBufferRef*> loaded;
	size_t indexUsed;

	explicit HWDevices(std::basic_ostream<char>& out): available(), loaded(), indexUsed(0) {
		AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
		while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE) {
			// I don't yet know why, but, if CUDA device is tested at a point and fails,
			// then all next hardware acceleration devices initializations will fail. So I will ignore CUDA device.
			if (type != AV_HWDEVICE_TYPE_CUDA)
				available.push_back(type);
		}
		out << "#MESSAGE Found " << available.size() << " hardware acceleration device(s)";
		if (!available.empty()) {
			out << ": " << av_hwdevice_get_type_name(available[0]);
			for (size_t i = 1; i < available.size(); ++i)
				out << ", " << av_hwdevice_get_type_name(available[i]);
		}
		out << '.' << std::endl;
	}

	~HWDevices() {
		for (auto it = loaded.begin(); it != loaded.end(); ++it)
			av_buffer_unref(&it->second);
	}
};

#endif //VIDEORAPTORBATCH_HWDEVICES_HPP
