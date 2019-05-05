//
// Created by notoraptor on 23/07/2018.
//

#ifndef VIDEORAPTORBATCH_STREAM_HPP
#define VIDEORAPTORBATCH_STREAM_HPP

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}
#include "HWDevices.hpp"
#include "VideoInfo.hpp"

struct Stream {
	int index;
	AVStream* stream;
	AVCodec* codec;
	AVCodecContext* codecContext;

	explicit Stream(): index(-1), stream(nullptr), codec(nullptr), codecContext(nullptr) {}

	void clear() {
		if (codecContext)
			avcodec_free_context(&codecContext);
	}
};

struct AudioStream: public Stream {
	VideoReport report;

	explicit AudioStream(): Stream(), report() {
		VideoReport_init(&report);
	}

	bool load(AVFormatContext* format) {
		if ((index = av_find_best_stream(format, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0)) < 0)
			return false;
		stream = format->streams[index];
		if (!(codecContext = avcodec_alloc_context3(codec)))
			return VideoReport_error(&report, ERROR_ALLOC_CODEC_CONTEXT);
		if (avcodec_parameters_to_context(codecContext, stream->codecpar) < 0)
			return VideoReport_error(&report, ERROR_CONVERT_CODEC_PARAMS);
		if (avcodec_open2(codecContext, codec, NULL) < 0)
			return VideoReport_error(&report, ERROR_OPEN_CODEC);
		return true;
	}
};

struct VideoStream: public Stream {
	const AVCodecHWConfig* selectedConfig;
	VideoReport* report;

private:
	bool loadHardwareDeviceConfig(AVHWDeviceType deviceType) {
		for (int i = 0;; ++i) {
			const AVCodecHWConfig* config = avcodec_get_hw_config(codec, i);
			if (!config) {
				return VideoReport_error(report, ERROR_FIND_HW_DEVICE_CONFIG);
			}
			if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == deviceType) {
				selectedConfig = config;
				return true;
			}
		}
	}

	bool loadHardwareDeviceContext(HWDevices& devices) {
		if (devices.loaded.find(selectedConfig->device_type) != devices.loaded.end()) {
			codecContext->hw_device_ctx = av_buffer_ref(devices.loaded[selectedConfig->device_type]);
			return true;
		}
		AVBufferRef* hwDeviceCtx = nullptr;
		if (av_hwdevice_ctx_create(&hwDeviceCtx, selectedConfig->device_type, NULL, NULL, 0) < 0) {
			return VideoReport_error(report, ERROR_CREATE_HW_DEVICE_CONFIG);
		}
		devices.loaded[selectedConfig->device_type] = hwDeviceCtx;
		codecContext->hw_device_ctx = av_buffer_ref(hwDeviceCtx);
		// For more info about created device context,
		// see av_hwdevice_get_type_name(selectedConfig->device_type)
		// and av_pix_fmt_desc_get(selectedConfig->pix_fmt)->name.
		return true;
	}

	static AVPixelFormat get_hw_format(AVCodecContext* ctx, const AVPixelFormat* pix_fmts) {
		auto streamInfo = (VideoStream*) ctx->opaque;
		if (streamInfo->selectedConfig) {
			for (const AVPixelFormat* p = pix_fmts; *p != -1; ++p) {
				if (*p == streamInfo->selectedConfig->pix_fmt)
					return streamInfo->selectedConfig->pix_fmt;
			}
		}
		VideoReport_error(streamInfo->report, ERROR_HW_SURFACE_FORMAT);
		return AV_PIX_FMT_NONE;
	}

public:

	explicit VideoStream(VideoReport* videoReport): Stream(), selectedConfig(nullptr), report(videoReport) {}

	bool load(AVFormatContext* format, HWDevices& devices, size_t deviceIndex) {
		if ((index = av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0)) < 0)
			return VideoReport_error(report, ERROR_FIND_VIDEO_STREAM);
		stream = format->streams[index];

		if (deviceIndex < devices.available.size() && !loadHardwareDeviceConfig(devices.available[deviceIndex]))
			return false;

		if (!(codecContext = avcodec_alloc_context3(codec)))
			return VideoReport_error(report, ERROR_ALLOC_CODEC_CONTEXT);
		if (avcodec_parameters_to_context(codecContext, stream->codecpar) < 0)
			return VideoReport_error(report, ERROR_CONVERT_CODEC_PARAMS);

		if (selectedConfig) {
			codecContext->opaque = this;
			codecContext->get_format = get_hw_format;
			av_opt_set_int(codecContext, "refcounted_frames", 1, 0);
			if (!loadHardwareDeviceContext(devices))
				return false;
		}

		if (avcodec_open2(codecContext, codec, NULL) < 0)
			return VideoReport_error(report, ERROR_OPEN_CODEC);

		if (codecContext->pix_fmt == AV_PIX_FMT_NONE)
			return VideoReport_error(report, ERROR_INVALID_PIX_FMT);
		if (codecContext->width <= 0)
			return VideoReport_error(report, ERROR_INVALID_WIDTH);
		if (codecContext->height <= 0)
			return VideoReport_error(report, ERROR_INVALID_HEIGHT);

		return true;
	}
};

#endif //VIDEORAPTORBATCH_STREAM_HPP
