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
#include "AppError.hpp"
#include "Stream.hpp"
#include "ThumbnailContext.hpp"
#include "VideoDetails.hpp"
#include "FileHandle.hpp"
#ifdef WIN32
#include "compatWindows.hpp"
#endif

#define THUMBNAIL_SIZE 300

class Video {
	FileHandle fileHandle;
	AVFormatContext* format;
	AVIOContext* avioContext;
	Stream audioStream;
	Stream videoStream;
	std::basic_ostream<char>& out;

	static std::string generateThumbnailPath(const std::string& thFolder, const std::string& thName) {
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

	bool openInput() {
#ifdef WIN32
		// Windows.
		return openCustomFormatContext(fileHandle, &format, &avioContext, out);
#else
		// Unix. TODO Test.
		if (avformat_open_input(&format, fileHandle.filename, NULL, NULL) != 0)
			return AppError(out, fileHandle.filename).write("Unable to open file.");
		return true;
#endif
	}

	bool savePNG(AVFrame* pFrame, const std::string& thFolder, const std::string& thName) {
		std::vector<unsigned char> image((size_t) (pFrame->width * pFrame->height * 4));

		// Write pixel data
		for (int y = 0; y < pFrame->height; ++y)
			memcpy(image.data() + (4 * pFrame->width * y), pFrame->data[0] + y * pFrame->linesize[0],
				   (size_t) (pFrame->width * 4));

		unsigned ret = lodepng::encode(
				generateThumbnailPath(thFolder, thName), image, (unsigned int) pFrame->width,
				(unsigned int) pFrame->height);
		if (ret) {
			return AppError(out, fileHandle.filename).write("PNG encoder error: ").write(lodepng_error_text(ret));
		}
		return true;
	}

public:

	explicit Video(std::basic_ostream<char>& output) :
			fileHandle(), format(nullptr), avioContext(nullptr),
			audioStream(output), videoStream(output), out(output) {}

	~Video() {
		if (avioContext) {
			av_freep(&avioContext->buffer);
			avio_context_free(&avioContext);
		}
		if (format) {
			videoStream.clear();
			audioStream.clear();
			avformat_close_input(&format);
		}
		if (fileHandle.file)
			fclose(fileHandle.file);
	}

	bool generateThumbnail(const std::string& thFolder, const std::string& thName) {
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
			return AppError(out, fileHandle.filename).write("Unable to seek into video.");

		// Read frames and save first video frame as image to disk
		while (av_read_frame(format, &thCtx.packet) >= 0) {
			// Is this a packet from the video stream?
			if (thCtx.packet.stream_index == videoStream.index) {
				// Send packet
				int ret = avcodec_send_packet(videoStream.codecContext, &thCtx.packet);
				if (ret < 0)
					return AppError(out, fileHandle.filename).write("Unable to send packet for decoding.");

				// Allocate video frame
				thCtx.frame = av_frame_alloc();
				if (thCtx.frame == NULL)
					return AppError(out, fileHandle.filename).write("Unable to allocate input frame.");

				// Receive frame.
				ret = avcodec_receive_frame(videoStream.codecContext, thCtx.frame);
				if (ret == AVERROR(EAGAIN))
					continue;
				if (ret == AVERROR_EOF || ret < 0)
					return AppError(out, fileHandle.filename).write("AppError while decoding video.");

				// Set frame to save (either from decoed frame or from GPU).
				if (videoStream.selectedConfig && thCtx.frame->format == videoStream.selectedConfig->pix_fmt) {
					// Allocate HW video frame
					thCtx.swFrame = av_frame_alloc();
					if (thCtx.swFrame == NULL)
						return AppError(out, fileHandle.filename).write("Unable to allocate HW input frame.");
					// retrieve data from GPU to CPU
					if (av_hwframe_transfer_data(thCtx.swFrame, thCtx.frame, 0) < 0)
						return AppError(out, fileHandle.filename).write("AppError transferring the data to system memory");
					thCtx.tmpFrame = thCtx.swFrame;
				} else {
					thCtx.tmpFrame = thCtx.frame;
				}

				// Allocate an AVFrame structure
				thCtx.frameRGB = av_frame_alloc();
				if (thCtx.frameRGB == NULL)
					return AppError(out, fileHandle.filename).write("Unable to allocate output frame.");

				// Determine required buffer size and allocate buffer
				numBytes = av_image_get_buffer_size(PIXEL_FMT, outputWidth, outputHeight, align);
				thCtx.buffer = (uint8_t*) av_malloc(numBytes * sizeof(uint8_t));
				if (!thCtx.buffer)
					return AppError(out, fileHandle.filename).write("Unable to allocate output frame buffer.");

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
				return savePNG(thCtx.frameRGB, thFolder, thName);
			}
		}

		return AppError(out, fileHandle.filename).write("Unable to save a thumbnail.");
	}

	bool load(const char* videoFilename, HWDevices& devices, size_t deviceIndex) {
		fileHandle.filename = videoFilename;
		// Open video file.
		if (!openInput())
			return false;
		// Retrieve stream information.
		if (avformat_find_stream_info(format, NULL) < 0)
			return AppError(out, fileHandle.filename).write("Unable to get streams info.");
		// Load best audio and video streams.
		if (!videoStream.load(fileHandle.filename, format, AVMEDIA_TYPE_VIDEO, devices, deviceIndex))
			return false;
		return audioStream.load(fileHandle.filename, format, AVMEDIA_TYPE_AUDIO, devices, deviceIndex) || audioStream.index < 0;
	}

	bool hasDeviceError() {
		return videoStream.deviceError;
	}

	void extractInfo(VideoDetails* videoDetails) {
		// clearDetails(videoDetails);
		initDetails(videoDetails);
		AVRational* frame_rate = &videoStream.stream->avg_frame_rate;
		if (!frame_rate->den)
			frame_rate = &videoStream.stream->r_frame_rate;
		videoDetails->filename = copyString(fileHandle.filename);
		videoDetails->duration = format->duration;
		videoDetails->duration_time_base = AV_TIME_BASE;
		videoDetails->size = avio_size(format->pb);
		videoDetails->container_format = copyString(format->iformat->long_name);
		videoDetails->width = videoStream.codecContext->width;
		videoDetails->height = videoStream.codecContext->height;
		videoDetails->video_codec = copyString(videoStream.codec->long_name);
		videoDetails->frame_rate_num = frame_rate->num;
		videoDetails->frame_rate_den = frame_rate->den;
		if (audioStream.index >= 0) {
			videoDetails->audio_codec = copyString(audioStream.codec->long_name);
			videoDetails->sample_rate = audioStream.codecContext->sample_rate;
			videoDetails->bit_rate = audioStream.codecContext->bit_rate;
		}
		if (AVDictionaryEntry* tag = av_dict_get(format->metadata, "title", NULL, AV_DICT_IGNORE_SUFFIX))
			videoDetails->title = copyString(tag->value);
	}
};

#endif //VIDEORAPTORBATCH_VIDEO_HPP