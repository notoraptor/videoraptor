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
#include "AppError.hpp"
#include "HWDevices.hpp"

struct Stream {
	int index;
	AVStream* stream;
	AVCodec* codec;
	AVCodecContext* codecContext;
	const AVCodecHWConfig* selectedConfig;
	bool deviceError;
	std::basic_ostream<char>& out;

	explicit Stream(std::basic_ostream<char>& output):
			index(-1), stream(nullptr), codec(nullptr), codecContext(nullptr),
			selectedConfig(nullptr), deviceError(false), out(output) {}

	bool loadDeviceConfigForCodec(const char* filename, AVHWDeviceType deviceType) {
		for (int i = 0;; ++i) {
			const AVCodecHWConfig* config = avcodec_get_hw_config(codec, i);
			if (!config) {
				deviceError = true;
				return AppWarning(out, filename)
						.write("Unable to find device config for device type ")
						.write(av_hwdevice_get_type_name(deviceType))
						.write(" and codec ")
						.write(codec->name);
			}
			if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == deviceType) {
				selectedConfig = config;
				return true;
			}
		}
	}

	int initHWDeviceContext(const char* filename, HWDevices& devices) {
		if (devices.loaded.find(selectedConfig->device_type) != devices.loaded.end()) {
			codecContext->hw_device_ctx = av_buffer_ref(devices.loaded[selectedConfig->device_type]);
//			out << "#MESSAGE Using already loaded device context "
//		  		<< av_hwdevice_get_type_name(selectedConfig->device_type)
//				<< " with pixel format " << selectedConfig->pix_fmt
//				<< " (" << av_pix_fmt_desc_get(selectedConfig->pix_fmt)->name << ")." << std::endl;
			return true;
		}
		AVBufferRef* hwDeviceCtx = nullptr;
		if (av_hwdevice_ctx_create(&hwDeviceCtx, selectedConfig->device_type, NULL, NULL, 0) < 0) {
			deviceError = true;
			return AppWarning(out, filename).write("Failed to create any HW device.");
		}
		devices.loaded[selectedConfig->device_type] = hwDeviceCtx;
		codecContext->hw_device_ctx = av_buffer_ref(hwDeviceCtx);
		out << "#MESSAGE Created device context "
			<< av_hwdevice_get_type_name(selectedConfig->device_type)
			<< " with pixel format " << selectedConfig->pix_fmt
			<< " (" << av_pix_fmt_desc_get(selectedConfig->pix_fmt)->name << ")." << std::endl;
		return true;
	}

	bool getHWFormat(const AVPixelFormat* pix_fmts) {
		if (selectedConfig) {
			for (const AVPixelFormat* p = pix_fmts; *p != -1; ++p) {
				if (*p == selectedConfig->pix_fmt)
					return true;
			}
		}

		return AppError(out).write("Failed to get HW surface format.");
	}

	bool load(const char* filename, AVFormatContext* format, AVMediaType type, HWDevices& devices, size_t deviceIndex) {
		if ((index = av_find_best_stream(format, type, -1, -1, &codec, 0)) < 0)
			return type == AVMEDIA_TYPE_VIDEO ? AppError(out, filename).write("Unable to find a video stream.") : false;
		stream = format->streams[index];

		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
			&& deviceIndex < devices.available.size()
			&& !loadDeviceConfigForCodec(filename, devices.available[deviceIndex]))
			return false;

		if (!(codecContext = avcodec_alloc_context3(codec)))
			return AppError(out, filename).write("Unable to alloc codec context.");
		if (avcodec_parameters_to_context(codecContext, stream->codecpar) < 0)
			return AppError(out, filename).write("Unable to convert codec parameters to context.");

		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && selectedConfig) {
			codecContext->opaque = this;
			codecContext->get_format = get_hw_format;
			av_opt_set_int(codecContext, "refcounted_frames", 1, 0);
			if (!initHWDeviceContext(filename, devices))
				return false;
		}

		if (avcodec_open2(codecContext, codec, NULL) < 0)
			return AppError(out, filename).write("Unable to open codec.");

		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			if (codecContext->pix_fmt == AV_PIX_FMT_NONE)
				return AppError(out, filename).write("Video stream has invalid pixel format.");
			if (codecContext->width <= 0)
				return AppError(out, filename).write("Video stream has invalid width.");
			if (codecContext->height <= 0)
				return AppError(out, filename).write("Video stream has invalid height.");
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
