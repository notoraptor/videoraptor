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

	explicit HWDevices(): available(), loaded(), indexUsed(0) {
		AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
		while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE) {
			// I don't yet know why, but, if CUDA device is tested at a point and fails,
			// then all next hardware acceleration devices initializations will fail. So I will ignore CUDA device.
			if (type != AV_HWDEVICE_TYPE_CUDA)
				available.push_back(type);
		}
	}

	~HWDevices() {
		for (auto it = loaded.begin(); it != loaded.end(); ++it)
			av_buffer_unref(&it->second);
	}

	size_t countDeviceTypes() const {
		return available.size();
	}

	size_t getStringRepresentationLength(const char* separator) const {
		if (available.empty())
			return 0;
		size_t totalSize = strlen(separator) * (available.size() - 1) + 1; // Number of times separator will be printed + terminal character \0.
		for (auto& deviceType : available) {
			totalSize += strlen(av_hwdevice_get_type_name(deviceType));
		}
		return totalSize;
	}

	void getStringRepresentation(char* output, const char* separator) const {
		if (available.empty())
			return;
		size_t len_separator = strlen(separator);
		const char* current_dt_name = av_hwdevice_get_type_name(available[0]);
		size_t len_current_dt_name = strlen(current_dt_name);
		memcpy(output, current_dt_name, len_current_dt_name);
		size_t cursor = len_current_dt_name;
		for (size_t i = 1; i < available.size(); ++i) {
			memcpy(output + cursor, separator, len_separator);
			cursor += len_separator;
			current_dt_name = av_hwdevice_get_type_name(available[i]);
			len_current_dt_name = strlen(current_dt_name);
			memcpy(output + cursor, current_dt_name, len_current_dt_name);
			cursor += len_current_dt_name;
		}
		output[cursor] = '\0';
	}
};

#endif //VIDEORAPTORBATCH_HWDEVICES_HPP
