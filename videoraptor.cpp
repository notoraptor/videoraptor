/** VideoRaptor: get and print video information using ffmpeg library.
 * #### References
 *   (2018/04/15): https://github.com/mpenkov/ffmpeg-tutorial/blob/master/tutorial01.c
 *   (2018/04/15): https://github.com/mikeboers/PyAV/blob/develop/av/container/input.pyx
 *   (2018/04/15): https://www.ffmpeg.org/doxygen/3.4/index.html
 *   (2018/04/29): https://stackoverflow.com/a/12563019
 * #### Compilation command (with ffmpeg 4.0)
 *   g++ videoraptor.cpp -o videoraptor
 *   -I <ffmpeg-include-dir> -L <ffmpeg-lib-dir>
 *   -lavcodec -lavformat -lavutil -lswscale
 * #### Usage (print help)
 *   videoraptor
 * #### Usage notes
 *   On Windows, you need FFMPEG DLLs: avcodec-*.dll, avformat-*.dll, avutil-*.dll, swresample-*.dll, swscale-*.dll.
 *   Some errors and warnings may be written in stderr:
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
#include <libavutil/log.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

#include <cstring>
#include <iostream>
#include <fstream>

#ifdef WIN32
const char separator = '\\', otherSeparator = '/';
#else
const char separator = '/', otherSeparator = '\\';
#endif
const char* const digits = "0123456789ABCDEF";
const char* globalCurrentFilename = nullptr;
const AVCodec* imageCodec = nullptr;

void customCallback(void* avcl, int level, const char* fmt, va_list vl) {
	if (level < av_log_get_level() && globalCurrentFilename) {
		char currentMessage[2048] = {};
		vsnprintf(currentMessage, sizeof(currentMessage), fmt, vl);
		for (char& character: currentMessage) {
			if (character == '\r' || character == '\n')
				character = ' ';
		}
		std::cout << "#VIDEO_WARNING[" << globalCurrentFilename << "]" << currentMessage << std::endl;
	}
}

inline bool error(const char* filename, const char* message) {
	std::cout << "#VIDEO_ERROR[" << filename << "]" << message << std::endl;
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

struct StreamInfo {
	int index;
	AVStream* stream;
	AVCodec* codec;
	AVCodecContext* codecContext;

	StreamInfo(): index(-1), stream(nullptr), codec(nullptr), codecContext(nullptr) {}

	bool load(const char* filename, AVFormatContext* format, enum AVMediaType type) {
		if ((index = av_find_best_stream(format, type, -1, -1, &codec, 0)) < 0)
			return false;
		stream = format->streams[index];
		codecContext = avcodec_alloc_context3(codec);
		if (!codecContext)
			return error(filename, "Unable to alloc codec context.");
		if (avcodec_parameters_to_context(codecContext, stream->codecpar) < 0)
			return error(filename, "Unable to convert codec parameters to context.");
		av_opt_set_int(codecContext, "refcounted_frames", 1, 0);
		if (avcodec_open2(codecContext, codec, NULL) < 0)
			return error(filename, "Unable to open codec.");
		return true;
	}

	void clear() { if (codecContext) avcodec_free_context(&codecContext); }
};

class VideoInfo {
	const char* filename;
	AVFormatContext* format;
	StreamInfo audioStream;
	StreamInfo videoStream;

	bool savePNG(AVFrame* frame, const std::string& thumbnailPath) {
		AVPacket pkt;
		AVCodecContext* imageCodecContext = avcodec_alloc_context3(imageCodec);
		if (!imageCodecContext)
			return error(filename, "Could not allocate PNG codec context.");
		imageCodecContext->time_base = (AVRational) {1, 25};
		imageCodecContext->width = videoStream.codecContext->width;
		imageCodecContext->height = videoStream.codecContext->height;
		imageCodecContext->pix_fmt = AV_PIX_FMT_RGB24;
		if (avcodec_open2(imageCodecContext, imageCodec, NULL) < 0)
			return error(filename, "Could not open PNG codec.");
		av_init_packet(&pkt);
		pkt.data = NULL;
		pkt.size = 0;
		if (avcodec_send_frame(imageCodecContext, frame) < 0)
			return error(filename, "Error sending frame for PNG encoding.");
		if (avcodec_receive_packet(imageCodecContext, &pkt) < 0)
			return error(filename, "Error receiving packet for PNG encoding.");
		FILE* f = fopen(thumbnailPath.c_str(), "wb");
		if (!f) return error(filename, "Unable to open thumbnail file for writing.");
		fwrite(pkt.data, 1, pkt.size, f);
		fclose(f);
		av_packet_unref(&pkt);
		avcodec_free_context(&imageCodecContext);
		return true;
	}

public:

	VideoInfo(): filename(nullptr), format(nullptr), audioStream(), videoStream() {}

	~VideoInfo() {
		if (format) {
			videoStream.clear();
			audioStream.clear();
			avformat_close_input(&format);
		}
	}

	bool load(const char* videoFilename) {
		filename = videoFilename;
		globalCurrentFilename = filename;
		// Open video file.
		if (avformat_open_input(&format, videoFilename, NULL, NULL) != 0)
			return error(filename, "Unable to open file.");
		// Retrieve stream information.
		if (avformat_find_stream_info(format, NULL) < 0)
			return error(filename, "Unable to get streams info.");
		// Load best audio and video streams.
		if (!videoStream.load(filename, format, AVMEDIA_TYPE_VIDEO))
			return error(filename, "Unable to load video stream.");
		if (!audioStream.load(filename, format, AVMEDIA_TYPE_AUDIO) && audioStream.index >= 0)
			return error(filename, "Unable to load audio stream found in video.");
		return true;
	}

	bool generateThumbnail(const std::string& thumbnailPath) {
		AVFrame* pFrame = NULL;
		AVFrame* pFrameRGB = NULL;
		uint8_t* buffer = NULL;
		struct SwsContext* sws_ctx = NULL;
		AVPacket packet;

		int numBytes;
		int saved = 0;
		int align = 32;

		// Allocate video frame
		pFrame = av_frame_alloc();
		if (pFrame == NULL)
			return error(filename, "Unable to allocate input frame.");

		// Allocate an AVFrame structure
		pFrameRGB = av_frame_alloc();
		if (pFrameRGB == NULL)
			return error(filename, "Unable to allocate output frame.");

		// Determine required buffer size and allocate buffer
		numBytes = av_image_get_buffer_size(
				AV_PIX_FMT_RGB24, videoStream.codecContext->width, videoStream.codecContext->height, align);
		buffer = (uint8_t*) av_malloc(numBytes * sizeof(uint8_t));
		if (!buffer)
			return error(filename, "Unable to allocate output frame buffer.");

		sws_ctx = sws_getContext(
				videoStream.codecContext->width, videoStream.codecContext->height, videoStream.codecContext->pix_fmt,
				videoStream.codecContext->width, videoStream.codecContext->height, AV_PIX_FMT_RGB24,
				SWS_BILINEAR, NULL, NULL, NULL
		);

		// Assign appropriate parts of buffer to image planes in pFrameRGB
		av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer,
							 AV_PIX_FMT_RGB24, videoStream.codecContext->width, videoStream.codecContext->height,
							 align);

		// seek
		if (av_seek_frame(format, -1, format->duration / 2, AVSEEK_FLAG_BACKWARD) < 0)
			return error(filename, "Unable to seek into video.");

		// Read frames and save first video frame as image to disk
		while (!saved && av_read_frame(format, &packet) >= 0) {
			// Is this a packet from the video stream?
			if (packet.stream_index == videoStream.index) {
				// Decode video frame
				int ret = avcodec_send_packet(videoStream.codecContext, &packet);
				if (ret < 0)
					return error(filename, "Unable to send packet for decoding.");
				ret = avcodec_receive_frame(videoStream.codecContext, pFrame);
				if (ret == AVERROR(EAGAIN))
					continue;
				if (ret == AVERROR_EOF || ret < 0)
					return error(filename, "Error while decoding video.");
				// Convert the image from its native format to RGB
				sws_scale(sws_ctx, (uint8_t const* const*) pFrame->data, pFrame->linesize, 0,
						  videoStream.codecContext->height, pFrameRGB->data, pFrameRGB->linesize);
				// Save the frame to disk
				pFrameRGB->width = videoStream.codecContext->width;
				pFrameRGB->height = videoStream.codecContext->height;
				pFrameRGB->format = AV_PIX_FMT_RGB24;
				if (!savePNG(pFrameRGB, thumbnailPath))
					return false;
				saved = 1;
			}
		}

		if (!saved)
			return error(filename, "Unable to save a thumbnail.");

		// Free buffer and frames
		av_packet_unref(&packet);
		av_free(buffer);
		av_frame_free(&pFrameRGB);
		av_frame_free(&pFrame);

		return true;
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

inline bool run(const char* videoFilename, const char* thumbFolder = nullptr, const char* thumbName = nullptr) {
	VideoInfo videoInfo;
	if (videoInfo.load(videoFilename)) {
		if (thumbFolder) {
			if (!thumbName)
				return error(videoFilename, "Cannot generate thumbnail without thumbnail name.");
			std::string thumbnailPath = thumbFolder;
			if (!thumbnailPath.empty()) {
				char lastChar = thumbnailPath[thumbnailPath.size() - 1];
				if (lastChar != separator && lastChar != otherSeparator)
					thumbnailPath.push_back(separator);
			}
			thumbnailPath += thumbName;
			thumbnailPath += ".png";
			for (char& character: thumbnailPath)
				if (character == otherSeparator)
					character = separator;
			// Generate thumbnail.
			videoInfo.generateThumbnail(thumbnailPath);
		} else {
			// Print video info.
			std::cout << videoInfo << std::endl;
		}
	}
	return true;
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
	av_log_set_callback(customCallback);
	// Load output image codec (for video thumbnails).
	imageCodec = avcodec_find_encoder(AV_CODEC_ID_PNG);
	if (!imageCodec) {
		std::cout << "#ERROR PNG codec not found." << std::endl;
		return -1;
	}
	// Parse arguments.
	if (extensionIsTxt(argv[1])) {
		std::cout << "#MESSAGE Given a TXT file." << std::endl;
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
				if (videoFilename && run(videoFilename, thumbnailFolder, thumbnailName)) {
					if ((++count) % 25 == 0)
						std::cout << "#LOADED " << count << std::endl;
				} else
					std::cout << "#IGNORED " << line << std::endl;
			}
		}
		std::cout << "#FINISHED " << count << std::endl;
	} else {
		std::cout << "#MESSAGE Given 1 movie file path." << std::endl;
		const char* videoFilename = argv[1];
		const char* thumbnailFolder = nullptr;
		const char* thumbnailName = nullptr;
		if (argc == 4) {
			thumbnailFolder = argv[2];
			thumbnailName = argv[3];
		}
		run(videoFilename, thumbnailFolder, thumbnailName);
	}

	return 0;
}
