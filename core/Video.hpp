//
// Created by notoraptor on 23/07/2018.
//

#ifndef VIDEORAPTORBATCH_VIDEO_HPP
#define VIDEORAPTORBATCH_VIDEO_HPP

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
};
#include <cstdio>
#include <sys/stat.h>
#include <lib/lodepng/lodepng.h>
#include "utils.hpp"
#include "unicode.hpp"
#include "Stream.hpp"
#include "ThumbnailContext.hpp"
#include "VideoInfo.hpp"
#include "VideoThumbnail.hpp"
#include "FileHandle.hpp"
#ifdef WIN32
#include "compatWindows.hpp"
#endif

#define THUMBNAIL_SIZE 300

class Video {
	FileHandle fileHandle;
	AVFormatContext* format;
	AVIOContext* avioContext;
	AudioStream audioStream;
	VideoStream videoStream;
	VideoReport* report;

	bool loadInputFile() {
#ifdef WIN32
		// Windows.
		return openCustomFormatContext(fileHandle, &format, &avioContext, report);
#else
		// Unix. TODO Test.
		if (avformat_open_input(&format, fileHandle.filename, NULL, NULL) != 0)
			return VideoReport_error(report, ERROR_OPEN_FILE);
		return true;
#endif
	}

	bool load(HWDevices& devices, size_t deviceIndex) {
		// Open video file.
		if (!loadInputFile())
			return false;
		// Retrieve stream information.
		if (avformat_find_stream_info(format, NULL) < 0)
			return VideoReport_error(report, ERROR_NO_STREAM_INFO);
		// Load best audio and video streams.
		if (!videoStream.load(format, devices, deviceIndex))
			return false;
		// Audio stream loading is optional.
		audioStream.load(format);
		return true;
	}

	static std::string generateThumbnailPath(const char* thFolder, const char* thName) {
		std::string thumbnailPath = thFolder;
		if (!thumbnailPath.empty()) {
			char lastChar = thumbnailPath[thumbnailPath.size() - 1];
			if (lastChar != SEPARATOR && lastChar != OTHER_SEPARATOR)
				thumbnailPath.push_back(SEPARATOR);
		}
		thumbnailPath += thName;
		thumbnailPath += ".png";
		for (char& character: thumbnailPath)
			if (character == OTHER_SEPARATOR)
				character = SEPARATOR;
		return thumbnailPath;
	}

	bool savePNG(AVFrame* pFrame, const char* thFolder, const char* thName) {
		std::vector<unsigned char> image((size_t) (pFrame->width * pFrame->height * 4));

		// Write pixel data
		for (int y = 0; y < pFrame->height; ++y)
			memcpy(image.data() + (4 * pFrame->width * y), pFrame->data[0] + y * pFrame->linesize[0],
					(size_t)pFrame->width * 4);

		unsigned ret = lodepng::encode(
				generateThumbnailPath(thFolder, thName), image, (unsigned int) pFrame->width,
				(unsigned int) pFrame->height);
		if (ret)
			return VideoReport_error(report, ERROR_PNG_ENCODER, lodepng_error_text(ret));

		return true;
	}

public:

	explicit Video(const char* filename, VideoReport* videoReport, HWDevices& devices, size_t deviceIndex) :
			fileHandle(filename), format(nullptr), avioContext(nullptr),
			audioStream(), videoStream(videoReport), report(videoReport) {
		load(devices, deviceIndex);
	}

	~Video() {
		if (avioContext) {
			av_freep(&avioContext->buffer);
			avio_context_free(&avioContext);
		}
		videoStream.clear();
		audioStream.clear();
		if (format) {
			avformat_close_input(&format);
		}
	}

	bool generateThumbnail(VideoThumbnail* videoThumbnail) {
		ThumbnailContext thCtx;

		int numBytes;
		int align = 32;
		int outputWidth = videoStream.codecContext->width;
		int outputHeight = videoStream.codecContext->height;
		if (outputWidth > THUMBNAIL_SIZE || outputHeight > THUMBNAIL_SIZE) {
			if (outputWidth > outputHeight) {
				outputHeight = THUMBNAIL_SIZE * outputHeight / outputWidth;
				outputWidth = THUMBNAIL_SIZE;
			} else if (outputWidth < outputHeight) {
				outputWidth = THUMBNAIL_SIZE * outputWidth / outputHeight;
				outputHeight = THUMBNAIL_SIZE;
			} else {
				outputWidth = outputHeight = THUMBNAIL_SIZE;
			}
		}

		// seek
		if (av_seek_frame(format, -1, format->duration / 2, AVSEEK_FLAG_BACKWARD) < 0)
			return VideoReport_error(report, ERROR_SEEK_VIDEO);

		// Read frames and save first video frame as image to disk
		while (av_read_frame(format, &thCtx.packet) >= 0) {
			// Is this a packet from the video stream?
			if (thCtx.packet.stream_index == videoStream.index) {
				// Send packet
				int ret = avcodec_send_packet(videoStream.codecContext, &thCtx.packet);
				if (ret < 0)
					return VideoReport_error(report, ERROR_SEND_PACKET);

				// Allocate video frame
				thCtx.frame = av_frame_alloc();
				if (thCtx.frame == NULL)
					return VideoReport_error(report, ERROR_ALLOC_INPUT_FRAME);

				// Receive frame.
				ret = avcodec_receive_frame(videoStream.codecContext, thCtx.frame);
				if (ret == AVERROR(EAGAIN))
					continue;
				if (ret == AVERROR_EOF || ret < 0)
					return VideoReport_error(report, ERROR_DECODE_VIDEO);

				// Set frame to save (either from decoded frame or from GPU).
				if (videoStream.selectedConfig && thCtx.frame->format == videoStream.selectedConfig->pix_fmt) {
					// Allocate HW video frame
					thCtx.swFrame = av_frame_alloc();
					if (thCtx.swFrame == NULL)
						return VideoReport_error(report, ERROR_ALLOC_HW_INPUT_FRAME);
					// retrieve data from GPU to CPU
					if (av_hwframe_transfer_data(thCtx.swFrame, thCtx.frame, 0) < 0)
						return VideoReport_error(report, ERROR_HW_DATA_TRANSFER);
					thCtx.tmpFrame = thCtx.swFrame;
				} else {
					thCtx.tmpFrame = thCtx.frame;
				}

				// Allocate an AVFrame structure
				thCtx.frameRGB = av_frame_alloc();
				if (thCtx.frameRGB == NULL)
					return VideoReport_error(report, ERROR_ALLOC_OUTPUT_FRAME);

				// Determine required buffer size and allocate buffer
				numBytes = av_image_get_buffer_size(PIXEL_FMT, outputWidth, outputHeight, align);
				thCtx.buffer = (uint8_t*) av_malloc(numBytes * sizeof(uint8_t));
				if (!thCtx.buffer)
					return VideoReport_error(report, ERROR_ALLOC_OUTPUT_FRAME_BUFFER);

				// Assign appropriate parts of buffer to image planes in frameRGB
				av_image_fill_arrays(thCtx.frameRGB->data, thCtx.frameRGB->linesize, thCtx.buffer,
									 PIXEL_FMT, outputWidth, outputHeight, align);

				thCtx.swsContext = sws_getContext(
						videoStream.codecContext->width, videoStream.codecContext->height,
						(AVPixelFormat) thCtx.tmpFrame->format,
						outputWidth, outputHeight, PIXEL_FMT, SWS_BILINEAR, NULL, NULL, NULL
				);

				// Convert the image from its native format to PIXEL_FMT
				sws_scale(thCtx.swsContext, (uint8_t const* const*) thCtx.tmpFrame->data, thCtx.tmpFrame->linesize, 0,
						  videoStream.codecContext->height, thCtx.frameRGB->data, thCtx.frameRGB->linesize);
				// Save the frame to disk
				thCtx.frameRGB->width = outputWidth;
				thCtx.frameRGB->height = outputHeight;
				thCtx.frameRGB->format = PIXEL_FMT;
				return VideoReport_setDone(report, savePNG(thCtx.frameRGB, videoThumbnail->thumbnailFolder, videoThumbnail->thumbnailName));
			}
		}

		return VideoReport_error(report, ERROR_SAVE_THUMBNAIL);
	}

	void extractInfo(VideoInfo* videoDetails) {
		AVRational* frame_rate = &videoStream.stream->avg_frame_rate;
		if (!frame_rate->den)
			frame_rate = &videoStream.stream->r_frame_rate;
		videoDetails->duration = format->duration;
		videoDetails->duration_time_base = AV_TIME_BASE;
		videoDetails->size = avio_size(format->pb);
		videoDetails->container_format = copyString(format->iformat->long_name);
		videoDetails->width = videoStream.codecContext->width;
		videoDetails->height = videoStream.codecContext->height;
		videoDetails->video_codec = copyString(videoStream.codec->name);
		videoDetails->video_codec_description = copyString(videoStream.codec->long_name);
		videoDetails->frame_rate_num = frame_rate->num;
		videoDetails->frame_rate_den = frame_rate->den;
		if (audioStream.index >= 0) {
			videoDetails->audio_codec = copyString(audioStream.codec->name);
			videoDetails->audio_codec_description = copyString(audioStream.codec->long_name);
			videoDetails->sample_rate = audioStream.codecContext->sample_rate;
			videoDetails->audio_bit_rate = audioStream.codecContext->bit_rate;
		}
		if (AVDictionaryEntry* tag = av_dict_get(format->metadata, "title", NULL, AV_DICT_IGNORE_SUFFIX))
			videoDetails->title = copyString(tag->value);
		VideoReport_setDone(&videoDetails->report, true);
	}
};

#endif //VIDEORAPTORBATCH_VIDEO_HPP
