/** VideoRaptor: get and print video information using ffmpeg library.
 * #### References
 *   (2018/04/15): https://github.com/mpenkov/ffmpeg-tutorial/blob/master/tutorial01.c
 *   (2018/04/15): https://github.com/mikeboers/PyAV/blob/develop/av/container/input.pyx
 *   (2018/04/15): https://www.ffmpeg.org/doxygen/trunk/index.html
 *   (2018/04/29): https://stackoverflow.com/a/12563019
 *   (2018/05/09): https://github.com/lvandeve/lodepng
 * #### Compilation command (with ffmpeg 4.0)
 *   g++ videoraptor.cpp lodepng.h lodepng.cpp -o videoraptor
 *   -I . -I <ffmpeg-include-dir> -L <ffmpeg-lib-dir> -lavcodec -lavformat -lavutil -lswscale -O3
 * #### Compilation command (windowe)
 *   g++ videoraptor.cpp lodepng.h lodepng.cpp -o .local\videoraptor
 *   -I . -I ..\ffmpeg-4.0-win64-dev\include -L ..\ffmpeg-4.0-win64-dev\lib -lavcodec -lavformat -lavutil -lswscale -O3
 * #### Usage (print help)
 *   videoraptor
 * #### Usage notes
 *   On Windows, you need FFMPEG DLLs: avcodec-*.dll, avformat-*.dll, avutil-*.dll, swresample-*.dll, swscale-*.dll.
 *   Some messages and warnings may be written in stdout:
 *     #ERROR <error_message>
 *     #FINISHED <integer>
 *     #IGNORED <video_filename>
 *     #LOADED <integer>
 *     #MESSAGE <message>
 *     #USAGE <help message>
 *     #VIDEO_ERROR[<video_filename>]<error_message>
 *     #VIDEO_WARNING[<video_filename>]<warning_message>
 * **/

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
}

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <lodepng.h>

#ifdef WIN32
const char separator = '\\', otherSeparator = '/';
#else
const char separator = '/', otherSeparator = '\\';
#endif
const char* const digits = "0123456789ABCDEF";
const AVPixelFormat PIXEL_FMT = AV_PIX_FMT_RGBA;
const AVCodec* imageCodec = nullptr;
const char* currentFilename = nullptr;

void customCallback(void* avcl, int level, const char* fmt, va_list vl) {
	if (level < av_log_get_level() && currentFilename) {
		char currentMessage[2048] = {};
		vsnprintf(currentMessage, sizeof(currentMessage), fmt, vl);
		for (char& character: currentMessage) {
			if (character == '\r' || character == '\n')
				character = ' ';
		}
		std::cout << "#VIDEO_WARNING[" << currentFilename << "]" << currentMessage << std::endl;
	}
}

struct HWDevices {
	std::vector<AVHWDeviceType> available;
	std::unordered_map<AVHWDeviceType, AVBufferRef*> loaded;
	size_t indexUsed;
	HWDevices(): available(), loaded(), indexUsed(0) {
		AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
		while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE) {
			/* I don't yet know why, but, if CUDA device is tested at a point and fail,
			 * then all next hardware acceleration devices initializations will fail.
			 * So I will ignore CUDA device. */
			if (type != AV_HWDEVICE_TYPE_CUDA)
				available.push_back(type);
		}
		std::cout << "#MESSAGE Found " << available.size() << " hardware acceleration device(s)";
		if (available.size()) {
			std::cout << ": " << av_hwdevice_get_type_name(available[0]);
			for (size_t i = 1; i < available.size(); ++i)
				std::cout << ", " << av_hwdevice_get_type_name(available[i]);
		}
		std::cout << '.' << std::endl;
	}
	~HWDevices() {
		for (auto it = loaded.begin(); it != loaded.end(); ++it)
			av_buffer_unref(&it->second);
	}
};

class Error {
	std::ostringstream oss;
public:
	explicit Error(const char* filename = nullptr): oss() {
		if (filename)
			oss << "#VIDEO_ERROR[" << filename << "]";
		else
			oss << "#ERROR ";
	}

	Error& write(const char* message) {
		oss << message;
		return *this;
	}

	operator bool() const {
		return false;
	}

	~Error() {
		std::cout << oss.str() << std::endl;
	}
};

enum class EncoderType { HEX, JSON };

struct StringEncoder {
	const char* str;
	EncoderType type;

	explicit StringEncoder(const char* s, EncoderType encoderType): str(s), type(encoderType) {}
};

inline std::ostream& operator<<(std::ostream& o, const StringEncoder& stringEncoder) {
	o << '"';
	const char* str = stringEncoder.str;
	switch (stringEncoder.type) {
		case EncoderType::HEX:
			while (*str) {
				unsigned char c = *str;
				o << digits[c >> 4] << digits[c & (((unsigned int) 1 << 4) - 1)];
				++str;
			}
			break;
		case EncoderType::JSON:
			while (*str) {
				if (*str == '"')
					o << "\\\"";
				else if (*str == '\\')
					o << "\\\\";
				else
					o << *str;
				++str;
			}
			break;
	}
	o << '"';
	return o;
}

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

struct StreamInfo {
	int index;
	AVStream* stream;
	AVCodec* codec;
	AVCodecContext* codecContext;

	const AVCodecHWConfig* selectedConfig;
	bool deviceError;

	StreamInfo(): index(-1), stream(nullptr), codec(nullptr), codecContext(nullptr),
				  deviceError(false), selectedConfig(nullptr) {}

	bool loadDeviceConfigForCodec(AVHWDeviceType deviceType) {
		for (int i = 0;; ++i) {
			const AVCodecHWConfig* config = avcodec_get_hw_config(codec, i);
			if (!config) {
				deviceError = true;
				return Error().write("Unable to find device config for codec ")
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
			std::cout << "#MESSAGE Using already loaded device context "
					  << av_hwdevice_get_type_name(selectedConfig->device_type)
					  << " with pixel format " << selectedConfig->pix_fmt <<
					  " (" << av_pix_fmt_desc_get(selectedConfig->pix_fmt)->name << ")." << std::endl;
			return true;
		}
		AVBufferRef* hwDeviceCtx = nullptr;
		if (av_hwdevice_ctx_create(&hwDeviceCtx, selectedConfig->device_type, NULL, NULL, 0) < 0) {
			deviceError = true;
			return Error().write("Failed to create any HW device.");
		}
		devices.loaded[selectedConfig->device_type] = hwDeviceCtx;
		codecContext->hw_device_ctx = av_buffer_ref(hwDeviceCtx);
		std::cout << "#MESSAGE Created device context "
				  << av_hwdevice_get_type_name(selectedConfig->device_type)
				  << " with pixel format " << selectedConfig->pix_fmt <<
				  " (" << av_pix_fmt_desc_get(selectedConfig->pix_fmt)->name << ")." << std::endl;
		return true;
	}

	bool getHWFormat(const AVPixelFormat* pix_fmts) {
		if (selectedConfig) {
			for (const AVPixelFormat* p = pix_fmts; *p != -1; ++p) {
				if (*p == selectedConfig->pix_fmt)
					return true;
			}
		}

		return Error().write("Failed to get HW surface format.");
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
			return Error(filename).write("Unable to alloc codec context.");
		if (avcodec_parameters_to_context(codecContext, stream->codecpar) < 0)
			return Error(filename).write("Unable to convert codec parameters to context.");

		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && selectedConfig) {
			codecContext->opaque = this;
			codecContext->get_format = get_hw_format;
			av_opt_set_int(codecContext, "refcounted_frames", 1, 0);
			if (!initHWDeviceContext(devices))
				return false;
		}

		if (avcodec_open2(codecContext, codec, NULL) < 0)
			return Error(filename).write("Unable to open codec.");

		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			if (codecContext->pix_fmt == AV_PIX_FMT_NONE)
				return Error(filename).write("Video stream has invalid pixel format.");
			if (codecContext->width <= 0)
				return Error(filename).write("Video stream has invalid width.");
			if (codecContext->height < 0)
				return Error(filename).write("Video stream has invalid height.");
		}
		return true;
	}

	void clear() {
		if (codecContext)
			avcodec_free_context(&codecContext);
	}

	static AVPixelFormat get_hw_format(AVCodecContext* ctx, const AVPixelFormat* pix_fmts) {
		StreamInfo* streamInfo = (StreamInfo*) ctx->opaque;
		if (!streamInfo->getHWFormat(pix_fmts)) {
			streamInfo->deviceError = true;
			return AV_PIX_FMT_NONE;
		}
		return streamInfo->selectedConfig->pix_fmt;
	}
};

class VideoInfo {
	const char* filename;
	AVFormatContext* format;
	StreamInfo audioStream;
	StreamInfo videoStream;

	bool savePNG(AVFrame* pFrame, const std::string& thumbnailPath) {
		std::vector<unsigned char> image((size_t) (pFrame->width * pFrame->height * 4));

		// Write pixel data
		for (unsigned int y = 0; y < pFrame->height; ++y)
			memcpy(image.data() + (4 * pFrame->width * y), pFrame->data[0] + y * pFrame->linesize[0],
				   (size_t) (pFrame->width * 4));

		unsigned ret = lodepng::encode(
				thumbnailPath, image, (unsigned int) pFrame->width, (unsigned int) pFrame->height);
		if (ret) {
			return Error(filename).write("PNG encoder error: ").write(lodepng_error_text(ret));
		}
		return true;
	}

public:

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
			return Error(filename).write("Unable to seek into video.");

		// Read frames and save first video frame as image to disk
		while (av_read_frame(format, &thCtx.packet) >= 0) {
			// Is this a packet from the video stream?
			if (thCtx.packet.stream_index == videoStream.index) {
				// Send packet
				int ret = avcodec_send_packet(videoStream.codecContext, &thCtx.packet);
				if (ret < 0)
					return Error(filename).write("Unable to send packet for decoding.");

				// Allocate video frame
				thCtx.frame = av_frame_alloc();
				if (thCtx.frame == NULL)
					return Error(filename).write("Unable to allocate input frame.");

				// Receive frame.
				ret = avcodec_receive_frame(videoStream.codecContext, thCtx.frame);
				if (ret == AVERROR(EAGAIN))
					continue;
				if (ret == AVERROR_EOF || ret < 0)
					return Error(filename).write("Error while decoding video.");

				// Set frame to save (either from decoed frame or from GPU).
				if (videoStream.selectedConfig && thCtx.frame->format == videoStream.selectedConfig->pix_fmt) {
					// Allocate HW video frame
					thCtx.swFrame = av_frame_alloc();
					if (thCtx.swFrame == NULL)
						return Error(filename).write("Unable to allocate HW input frame.");
					/* retrieve data from GPU to CPU */
					if (av_hwframe_transfer_data(thCtx.swFrame, thCtx.frame, 0) < 0)
						return Error(filename).write("Error transferring the data to system memory");
					thCtx.tmpFrame = thCtx.swFrame;
				} else {
					thCtx.tmpFrame = thCtx.frame;
				}

				// Allocate an AVFrame structure
				thCtx.frameRGB = av_frame_alloc();
				if (thCtx.frameRGB == NULL)
					return Error(filename).write("Unable to allocate output frame.");

				// Determine required buffer size and allocate buffer
				numBytes = av_image_get_buffer_size(PIXEL_FMT, outputWidth, outputHeight, align);
				thCtx.buffer = (uint8_t*) av_malloc(numBytes * sizeof(uint8_t));
				if (!thCtx.buffer)
					return Error(filename).write("Unable to allocate output frame buffer.");

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

		return Error(filename).write("Unable to save a thumbnail.");
	}

	bool load(const char* videoFilename, HWDevices& devices, size_t deviceIndex) {
		filename = videoFilename;
		currentFilename = filename;
		// Open video file.
		if (avformat_open_input(&format, videoFilename, NULL, NULL) != 0)
			return Error(filename).write("Unable to open file.");
		// Retrieve stream information.
		if (avformat_find_stream_info(format, NULL) < 0)
			return Error(filename).write("Unable to get streams info.");
		// Load best audio and video streams.
		if (!videoStream.load(filename, format, AVMEDIA_TYPE_VIDEO, devices, deviceIndex))
			return Error(filename).write("Unable to load video stream.");
		if (!audioStream.load(filename, format, AVMEDIA_TYPE_AUDIO, devices, deviceIndex) && audioStream.index >= 0)
			return Error(filename).write("Unable to load audio stream.");
		return true;
	}

	bool hasDeviceError() {
		return videoStream.deviceError;
	}

	VideoInfo(): filename(nullptr), format(nullptr), audioStream(), videoStream() {}

	~VideoInfo() {
		if (format) {
			videoStream.clear();
			audioStream.clear();
			avformat_close_input(&format);
		}
	}

	friend std::ostream& operator<<(std::ostream& o, const VideoInfo& videoInfo);
};

inline std::ostream& operator<<(std::ostream& o, const VideoInfo& videoInfo) {
	AVRational* frame_rate = &videoInfo.videoStream.stream->avg_frame_rate;
	if (!frame_rate->den)
		frame_rate = &videoInfo.videoStream.stream->r_frame_rate;
	o << '{';
	o << R"("filename":)" << StringEncoder(videoInfo.filename, EncoderType::JSON) << ',';
	o << R"("duration":)" << videoInfo.format->duration << ',';
	o << R"("duration_time_base":)" << AV_TIME_BASE << ',';
	o << R"("size":)" << avio_size(videoInfo.format->pb) << ',';
	o << R"("container_format":)" << StringEncoder(videoInfo.format->iformat->long_name, EncoderType::JSON) << ',';
	o << R"("width":)" << videoInfo.videoStream.codecContext->width << ',';
	o << R"("height":)" << videoInfo.videoStream.codecContext->height << ',';
	o << R"("video_codec":)" << StringEncoder(videoInfo.videoStream.codec->long_name, EncoderType::JSON) << ',';
	o << R"("frame_rate":")" << frame_rate->num << '/' << frame_rate->den << '"';
	if (videoInfo.audioStream.index >= 0) {
		o << ',';
		o << R"("audio_codec":)" << StringEncoder(videoInfo.audioStream.codec->long_name, EncoderType::JSON) << ',';
		o << R"("sample_rate":)" << videoInfo.audioStream.codecContext->sample_rate << ',';
		o << R"("bit_rate":)" << videoInfo.audioStream.codecContext->bit_rate;
	}
	AVDictionaryEntry* tag = av_dict_get(videoInfo.format->metadata, "title", NULL, AV_DICT_IGNORE_SUFFIX);
	if (tag)
		o << ',' << R"("title":)" << StringEncoder(tag->value, EncoderType::HEX);
	o << '}';
	return o;
};

inline bool run(HWDevices& devices, const char* filename, const char* thFolder = nullptr, const char* thName = nullptr) {
	size_t deviceIndex = devices.indexUsed;
	do {
		VideoInfo videoInfo;
		if (!videoInfo.load(filename, devices, deviceIndex)) {
			if (videoInfo.hasDeviceError()) {
				std::cout << "#WARNING Device error." << std::endl;
				++deviceIndex;
				continue;
			}
			return false;
		}
		devices.indexUsed = deviceIndex;
		if (thFolder) {
			if (!thName)
				return Error(filename).write("Cannot generate thumbnail without thumbnail name.");
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
				if (videoInfo.hasDeviceError())
					continue;
				return false;
			};
		} else {
			// Print video info.
			std::cout << videoInfo << std::endl;
		}
		return true;
	} while (deviceIndex < devices.available.size());
	return false;
}

inline void stripString(std::string& s) {
	size_t stripLeft = std::string::npos;
	for (size_t i = 0; i < s.length(); ++i) {
		if (!std::isspace(s[i])) {
			stripLeft = i;
			break;
		}
	}
	if (stripLeft == std::string::npos) {
		s.clear();
	} else {
		s.erase(0, stripLeft);
		for (size_t i = s.length(); i > 0; --i) {
			if (!std::isspace(s[i - 1])) {
				if (i != s.length())
					s.erase(i);
				break;
			}
		}
	}
}

inline bool extensionIsTxt(const char* s) {
	size_t len = strlen(s);
	return len >= 4 && s[len - 4] == '.' && (s[len - 3] == 't' || s[len - 3] == 'T')
		   && (s[len - 2] == 'x' || s[len - 2] == 'X') && (s[len - 1] == 't' || s[len - 1] == 'T');
}

int help() {
	std::cout << "#USAGE SINGLE VIDEO COMMAND:" << std::endl;
	std::cout << std::endl;
	std::cout << "#USAGE videoraptor <video_filename>" << std::endl;
	std::cout << "#USAGE     Print info about given video file." << std::endl;
	std::cout << std::endl;
	std::cout << "#USAGE videoraptor <video_filename> <thumbnail_folder> <thumbnail_title>" << std::endl;
	std::cout << "#USABE     Generate <thumbnail_folder>/<thumbnail_title>.png from given video file." << std::endl;
	std::cout << std::endl;
	std::cout << "#USAGE BATCH COMMAND:" << std::endl;
	std::cout << std::endl;
	std::cout << "#USAGE videoraptor <txt_filename>" << std::endl;
	std::cout << "#USAGE     Parse given text file. Each line must be a single video command" << std::endl;
	std::cout << "#USAGE     with arguments separated by a tab character." << std::endl;
	return -1;
}

int main(int argc, char* argv[]) {
	// Check if there are arguments.
	if (argc != 2 && argc != 4)
		return help();
	// Initializations.
	av_log_set_callback(customCallback);
	HWDevices devices;
	// Load output image codec (for video thumbnails).
	imageCodec = avcodec_find_encoder(AV_CODEC_ID_PNG);
	if (!imageCodec) {
		std::cout << "#ERROR PNG codec not found." << std::endl;
		return -1;
	}
	// Parse arguments.
	if (extensionIsTxt(argv[1])) {
		std::ifstream textFile(argv[1]);
		std::string line;
		size_t count = 0;
		while (std::getline(textFile, line)) {
			stripString(line);
			if (!line.empty() && line[0] != '#') {
				size_t posTab1 = line.find('\t', 0);
				size_t posTab2 = (posTab1 != std::string::npos) ? line.find('\t', posTab1 + 1) : std::string::npos;
				const char* videoFilename = line.c_str();
				const char* thumbnailFolder = nullptr;
				const char* thumbnailName = nullptr;
				if (posTab1 != std::string::npos) {
					if (posTab2 == std::string::npos) {
						std::cout << "#ERROR line " << line << std::endl;
						std::cout << "#ERROR Bad syntax (expected 3 columns separated by a tab)." << std::endl;
						videoFilename = nullptr;
					} else {
						line[posTab1] = line[posTab2] = '\0';
						thumbnailFolder = line.c_str() + posTab1 + 1;
						thumbnailName = line.c_str() + posTab2 + 1;
					}
				}
				if (videoFilename && run(devices, videoFilename, thumbnailFolder, thumbnailName)) {
					if ((++count) % 25 == 0)
						std::cout << "#LOADED " << count << std::endl;
				} else
					std::cout << "#IGNORED " << line << std::endl;
			}
		}
		std::cout << "#FINISHED " << count << std::endl;
	} else {
		const char* videoFilename = argv[1];
		const char* thumbnailFolder = nullptr;
		const char* thumbnailName = nullptr;
		if (argc == 4) {
			thumbnailFolder = argv[2];
			thumbnailName = argv[3];
		}
		run(devices, videoFilename, thumbnailFolder, thumbnailName);
	}

	return 0;
}
