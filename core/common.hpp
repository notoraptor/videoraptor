//
// Created by notoraptor on 27/05/2018.
//

#ifndef VIDEORAPTOR_COMMON_HPP
#define VIDEORAPTOR_COMMON_HPP

#include <unordered_map>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
};
#include <core/AppError.hpp>
#include <core/StringEncoder.hpp>
#include <core/VideoDetails.hpp>
#include <lodepng/lodepng.h>

extern const char separator;
extern const char otherSeparator;
extern const AVPixelFormat PIXEL_FMT;
extern const AVCodec* imageCodec;
extern const char* currentFilename;

void customCallback(void* avcl, int level, const char* fmt, va_list vl);

struct HWDevices {
	std::vector<AVHWDeviceType> available;
	std::unordered_map<AVHWDeviceType, AVBufferRef*> loaded;
	size_t indexUsed;

	explicit HWDevices(std::basic_ostream<char>& out): available(), loaded(), indexUsed(0) {
		AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
		while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE) {
			/* I don't yet know why, but, if CUDA device is tested at a point and fail,
			 * then all next hardware acceleration devices initializations will fail.
			 * So I will ignore CUDA device. */
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

struct ThumbnailContext {
	AVFrame* tmpFrame; // either frame or swFrame.
	AVFrame* frame;
	AVFrame* swFrame;
	AVFrame* frameRGB;
	uint8_t* buffer;
	SwsContext* swsContext;
	AVPacket packet;
	bool packetIsUsed;

	ThumbnailContext(): frame(nullptr), swFrame(nullptr), tmpFrame(nullptr), frameRGB(nullptr), buffer(nullptr),
						swsContext(nullptr), packet(), packetIsUsed(false) {}

	~ThumbnailContext() {
		if (packetIsUsed)
			av_packet_unref(&packet);
		if (frame)
			av_frame_free(&frame);
		if (swFrame)
			av_frame_free(&swFrame);
		if (frameRGB)
			av_frame_free(&frameRGB);
		if (buffer)
			av_freep(&buffer);
		if (swsContext)
			sws_freeContext(swsContext);
	}

};

struct Stream {
	int index;
	AVStream* stream;
	AVCodec* codec;
	AVCodecContext* codecContext;
	std::basic_ostream<char>& out;

	const AVCodecHWConfig* selectedConfig;
	bool deviceError;

	explicit Stream(std::basic_ostream<char>& output):
			index(-1), stream(nullptr), codec(nullptr), codecContext(nullptr),
			deviceError(false), selectedConfig(nullptr), out(output) {}

	bool loadDeviceConfigForCodec(AVHWDeviceType deviceType) {
		for (int i = 0;; ++i) {
			const AVCodecHWConfig* config = avcodec_get_hw_config(codec, i);
			if (!config) {
				deviceError = true;
				return AppError(out).write("Unable to find device config for codec ")
						.write(av_hwdevice_get_type_name(deviceType));
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
			out << "#MESSAGE Using already loaded device context "
		  		<< av_hwdevice_get_type_name(selectedConfig->device_type)
				<< " with pixel format " << selectedConfig->pix_fmt
				<< " (" << av_pix_fmt_desc_get(selectedConfig->pix_fmt)->name << ")." << std::endl;
			return true;
		}
		AVBufferRef* hwDeviceCtx = nullptr;
		if (av_hwdevice_ctx_create(&hwDeviceCtx, selectedConfig->device_type, NULL, NULL, 0) < 0) {
			deviceError = true;
			return AppError(out).write("Failed to create any HW device.");
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
			return false;
		stream = format->streams[index];
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
			&& deviceIndex < devices.available.size()
			&& !loadDeviceConfigForCodec(devices.available[deviceIndex])) {
			return false;
		}
		if (!(codecContext = avcodec_alloc_context3(codec)))
			return AppError(out, filename).write("Unable to alloc codec context.");
		if (avcodec_parameters_to_context(codecContext, stream->codecpar) < 0)
			return AppError(out, filename).write("Unable to convert codec parameters to context.");

		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && selectedConfig) {
			codecContext->opaque = this;
			codecContext->get_format = get_hw_format;
			av_opt_set_int(codecContext, "refcounted_frames", 1, 0);
			if (!initHWDeviceContext(devices))
				return false;
		}

		if (avcodec_open2(codecContext, codec, NULL) < 0)
			return AppError(out, filename).write("Unable to open codec.");

		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			if (codecContext->pix_fmt == AV_PIX_FMT_NONE)
				return AppError(out, filename).write("Video stream has invalid pixel format.");
			if (codecContext->width <= 0)
				return AppError(out, filename).write("Video stream has invalid width.");
			if (codecContext->height < 0)
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

class Video {
	const char* filename;
	AVFormatContext* format;
	Stream audioStream;
	Stream videoStream;
	std::basic_ostream<char>& out;

	bool savePNG(AVFrame* pFrame, const std::string& thumbnailPath) {
		std::vector<unsigned char> image((size_t) (pFrame->width * pFrame->height * 4));

		// Write pixel data
		for (unsigned int y = 0; y < pFrame->height; ++y)
			memcpy(image.data() + (4 * pFrame->width * y), pFrame->data[0] + y * pFrame->linesize[0],
				   (size_t) (pFrame->width * 4));

		unsigned ret = lodepng::encode(
				thumbnailPath, image, (unsigned int) pFrame->width, (unsigned int) pFrame->height);
		if (ret) {
			return AppError(out, filename).write("PNG encoder error: ").write(lodepng_error_text(ret));
		}
		return true;
	}

public:

	explicit Video(std::basic_ostream<char>& output):
			filename(nullptr), format(nullptr), audioStream(output), videoStream(output), out(output) {}

	~Video() {
		if (format) {
			videoStream.clear();
			audioStream.clear();
			avformat_close_input(&format);
		}
	}

	bool generateThumbnail(const std::string& thumbnailPath) {
		ThumbnailContext thCtx;

		int numBytes;
		int align = 32;
		int outputWidth = videoStream.codecContext->width / 4;
		int outputHeight = videoStream.codecContext->height / 4;
		if (!outputWidth || !outputHeight) {
			outputWidth = videoStream.codecContext->width;
			outputHeight = videoStream.codecContext->height;
		}

		// seek
		if (av_seek_frame(format, -1, format->duration / 2, AVSEEK_FLAG_BACKWARD) < 0)
			return AppError(out, filename).write("Unable to seek into video.");

		// Read frames and save first video frame as image to disk
		while (av_read_frame(format, &thCtx.packet) >= 0) {
			// Is this a packet from the video stream?
			if (thCtx.packet.stream_index == videoStream.index) {
				// Send packet
				int ret = avcodec_send_packet(videoStream.codecContext, &thCtx.packet);
				if (ret < 0)
					return AppError(out, filename).write("Unable to send packet for decoding.");

				// Allocate video frame
				thCtx.frame = av_frame_alloc();
				if (thCtx.frame == NULL)
					return AppError(out, filename).write("Unable to allocate input frame.");

				// Receive frame.
				ret = avcodec_receive_frame(videoStream.codecContext, thCtx.frame);
				if (ret == AVERROR(EAGAIN))
					continue;
				if (ret == AVERROR_EOF || ret < 0)
					return AppError(out, filename).write("AppError while decoding video.");

				// Set frame to save (either from decoed frame or from GPU).
				if (videoStream.selectedConfig && thCtx.frame->format == videoStream.selectedConfig->pix_fmt) {
					// Allocate HW video frame
					thCtx.swFrame = av_frame_alloc();
					if (thCtx.swFrame == NULL)
						return AppError(out, filename).write("Unable to allocate HW input frame.");
					/* retrieve data from GPU to CPU */
					if (av_hwframe_transfer_data(thCtx.swFrame, thCtx.frame, 0) < 0)
						return AppError(out, filename).write("AppError transferring the data to system memory");
					thCtx.tmpFrame = thCtx.swFrame;
				} else {
					thCtx.tmpFrame = thCtx.frame;
				}

				// Allocate an AVFrame structure
				thCtx.frameRGB = av_frame_alloc();
				if (thCtx.frameRGB == NULL)
					return AppError(out, filename).write("Unable to allocate output frame.");

				// Determine required buffer size and allocate buffer
				numBytes = av_image_get_buffer_size(PIXEL_FMT, outputWidth, outputHeight, align);
				thCtx.buffer = (uint8_t*) av_malloc(numBytes * sizeof(uint8_t));
				if (!thCtx.buffer)
					return AppError(out, filename).write("Unable to allocate output frame buffer.");

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
				return savePNG(thCtx.frameRGB, thumbnailPath);
			}
		}

		return AppError(out, filename).write("Unable to save a thumbnail.");
	}

	bool load(const char* videoFilename, HWDevices& devices, size_t deviceIndex) {
		filename = videoFilename;
		currentFilename = filename;
		// Open video file.
		if (avformat_open_input(&format, videoFilename, NULL, NULL) != 0)
			return AppError(out, filename).write("Unable to open file.");
		// Retrieve stream information.
		if (avformat_find_stream_info(format, NULL) < 0)
			return AppError(out, filename).write("Unable to get streams info.");
		// Load best audio and video streams.
		if (!videoStream.load(filename, format, AVMEDIA_TYPE_VIDEO, devices, deviceIndex))
			return AppError(out, filename).write("Unable to load video stream.");
		if (!audioStream.load(filename, format, AVMEDIA_TYPE_AUDIO, devices, deviceIndex) && audioStream.index >= 0)
			return AppError(out, filename).write("Unable to load audio stream.");
		return true;
	}

	bool hasDeviceError() {
		return videoStream.deviceError;
	}

	void extractInfo(VideoDetails* videoDetails) {
		clearDetails(videoDetails);
		initDetails(videoDetails);
		AVRational* frame_rate = &videoStream.stream->avg_frame_rate;
		if (!frame_rate->den)
			frame_rate = &videoStream.stream->r_frame_rate;
		videoDetails->filename = copyString(filename);
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
			videoDetails->title = StringEncoder(tag->value, EncoderType::HEX).encode();
	}

	friend std::ostream& operator<<(std::ostream&, const Video&);
};

inline std::ostream& operator<<(std::ostream& o, const Video& video) {
	AVRational* frame_rate = &video.videoStream.stream->avg_frame_rate;
	if (!frame_rate->den)
		frame_rate = &video.videoStream.stream->r_frame_rate;
	o << '{';
	o << R"("filename":)" << StringEncoder(video.filename, EncoderType::JSON) << ',';
	o << R"("duration":)" << video.format->duration << ',';
	o << R"("duration_time_base":)" << AV_TIME_BASE << ',';
	o << R"("size":)" << avio_size(video.format->pb) << ',';
	o << R"("container_format":)" << StringEncoder(video.format->iformat->long_name, EncoderType::JSON) << ',';
	o << R"("width":)" << video.videoStream.codecContext->width << ',';
	o << R"("height":)" << video.videoStream.codecContext->height << ',';
	o << R"("video_codec":)" << StringEncoder(video.videoStream.codec->long_name, EncoderType::JSON) << ',';
	o << R"("frame_rate":")" << frame_rate->num << '/' << frame_rate->den << '"';
	if (video.audioStream.index >= 0) {
		o << ',';
		o << R"("audio_codec":)" << StringEncoder(video.audioStream.codec->long_name, EncoderType::JSON) << ',';
		o << R"("sample_rate":)" << video.audioStream.codecContext->sample_rate << ',';
		o << R"("bit_rate":)" << video.audioStream.codecContext->bit_rate;
	}
	AVDictionaryEntry* tag = av_dict_get(video.format->metadata, "title", NULL, AV_DICT_IGNORE_SUFFIX);
	if (tag)
		o << ',' << R"("title":)" << StringEncoder(tag->value, EncoderType::HEX);
	o << '}';
	return o;
};

inline bool run(HWDevices& devices, std::basic_ostream<char>& out,
				const char* filename, const char* thFolder, const char* thName) {
	size_t deviceIndex = devices.indexUsed;
	while (deviceIndex < devices.available.size()) {
		Video videoInfo(out);
		if (!videoInfo.load(filename, devices, deviceIndex)) {
			if (videoInfo.hasDeviceError()) {
				out << "#WARNING Device error when loading video." << std::endl;
				++deviceIndex;
				continue;
			}
			return false;
		}
		devices.indexUsed = deviceIndex;
		if (!thFolder) {
			out << videoInfo << std::endl; // Print video info.
		} else {
			if (!thName)
				return AppError(out, filename).write("Cannot generate thumbnail without thumbnail name.");
			std::string thumbnailPath = thFolder;
			if (!thumbnailPath.empty()) {
				char lastChar = thumbnailPath[thumbnailPath.size() - 1];
				if (lastChar != separator && lastChar != otherSeparator)
					thumbnailPath.push_back(separator);
			}
			thumbnailPath += thName;
			thumbnailPath += ".png";
			for (char& character: thumbnailPath)
				if (character == otherSeparator)
					character = separator;
			// Generate thumbnail.
			if (!videoInfo.generateThumbnail(thumbnailPath)) {
				if (videoInfo.hasDeviceError()) {
					out << "#WARNING Device error when generating thumbnail." << std::endl;
					++deviceIndex;
					continue;
				}
				return false;
			};
		}
		return true;
	};
	return false;
}

inline bool getVideoDetails(HWDevices& devices, std::basic_ostream<char>& out,
							const char* filename, VideoDetails* videoDetails) {
	size_t deviceIndex = devices.indexUsed;
	while (deviceIndex < devices.available.size()) {
		Video videoInfo(out);
		if (!videoInfo.load(filename, devices, deviceIndex)) {
			if (videoInfo.hasDeviceError()) {
				out << "#WARNING Device error when loading video." << std::endl;
				++deviceIndex;
				continue;
			}
			return false;
		}
		devices.indexUsed = deviceIndex;
		videoInfo.extractInfo(videoDetails);
		return true;
	};
	return false;
}

HWDevices* initVideoRaptor(std::basic_ostream<char>& out);

std::ostringstream& videoRaptorLogger(char**);
void lockLogger();
void unlockLogger();

#endif //VIDEORAPTOR_COMMON_HPP
