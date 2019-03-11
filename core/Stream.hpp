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
};
#include "HWDevices.hpp"
#include "VideoDetails.hpp"
#include "errorCodes.hpp"

struct Stream {
	int index;
	AVStream* stream;
	AVCodec* codec;
	AVCodecContext* codecContext;
	const AVCodecHWConfig* selectedConfig;
	bool deviceError;
	VideoErrors* errors;

	explicit Stream(VideoErrors* videoErrors):
			index(-1), stream(nullptr), codec(nullptr), codecContext(nullptr),
			selectedConfig(nullptr), deviceError(false), errors(videoErrors) {}

	bool loadDeviceConfigForCodec(AVHWDeviceType deviceType) {
		for (int i = 0;; ++i) {
			const AVCodecHWConfig* config = avcodec_get_hw_config(codec, i);
			if (!config) {
				deviceError = true;
				return videoRaptorError(errors, WARNING_FIND_DEVICE_CONFIG);
			}
			if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == deviceType) {
				selectedConfig = config;
				return true;
			}
		}
	}

	int initHWDeviceContext(HWDevices& devices) {
		if (devices.loaded.find(selectedConfig->device_type) != devices.loaded.end()) {
			codecContext->hw_device_ctx = av_buffer_ref(devices.loaded[selectedConfig->device_type]);
			return true;
		}
		AVBufferRef* hwDeviceCtx = nullptr;
		if (av_hwdevice_ctx_create(&hwDeviceCtx, selectedConfig->device_type, NULL, NULL, 0) < 0) {
			deviceError = true;
			return videoRaptorError(errors, WARNING_CREATE_DEVICE_CONFIG);
		}
		devices.loaded[selectedConfig->device_type] = hwDeviceCtx;
		codecContext->hw_device_ctx = av_buffer_ref(hwDeviceCtx);
		// For more info about created device context, see av_hwdevice_get_type_name(selectedConfig->device_type)
		// and av_pix_fmt_desc_get(selectedConfig->pix_fmt)->name.
		return true;
	}

	bool getHWFormat(const AVPixelFormat* pix_fmts) {
		if (selectedConfig) {
			for (const AVPixelFormat* p = pix_fmts; *p != -1; ++p) {
				if (*p == selectedConfig->pix_fmt)
					return true;
			}
		}

		return videoRaptorError(errors, ERROR_HW_SURFACE_FORMAT);
	}

	bool load(AVFormatContext* format, AVMediaType type, HWDevices& devices, size_t deviceIndex) {
		if ((index = av_find_best_stream(format, type, -1, -1, &codec, 0)) < 0)
			return type == AVMEDIA_TYPE_VIDEO ? videoRaptorError(errors, ERROR_FIND_VIDEO_STREAM) : false;
		stream = format->streams[index];

		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
			&& deviceIndex < devices.available.size()
			&& !loadDeviceConfigForCodec(devices.available[deviceIndex]))
			return false;

		if (!(codecContext = avcodec_alloc_context3(codec)))
			return videoRaptorError(errors, ERROR_ALLOC_CODEC_CONTEXT);
		if (avcodec_parameters_to_context(codecContext, stream->codecpar) < 0)
			return videoRaptorError(errors, ERROR_CONVERT_CODEC_PARAMS);

		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && selectedConfig) {
			codecContext->opaque = this;
			codecContext->get_format = get_hw_format;
			av_opt_set_int(codecContext, "refcounted_frames", 1, 0);
			if (!initHWDeviceContext(devices))
				return false;
		}

		if (avcodec_open2(codecContext, codec, NULL) < 0)
			return videoRaptorError(errors, ERROR_OPEN_CODEC);

		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			if (codecContext->pix_fmt == AV_PIX_FMT_NONE)
				return videoRaptorError(errors, ERROR_INVALID_PIX_FMT);
			if (codecContext->width <= 0)
				return videoRaptorError(errors, ERROR_INVALID_WIDTH);
			if (codecContext->height <= 0)
				return videoRaptorError(errors, ERROR_INVALID_HEIGHT);
		}

		return true;
	}

	void clear() {
		if (codecContext)
			avcodec_free_context(&codecContext);
	}

	static AVPixelFormat get_hw_format(AVCodecContext* ctx, const AVPixelFormat* pix_fmts) {
		Stream* streamInfo = (Stream*) ctx->opaque;
		if (!streamInfo->getHWFormat(pix_fmts)) {
			streamInfo->deviceError = true;
			return AV_PIX_FMT_NONE;
		}
		return streamInfo->selectedConfig->pix_fmt;
	}
};

#endif //VIDEORAPTORBATCH_STREAM_HPP
