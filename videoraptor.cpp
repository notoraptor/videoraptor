/** VideoRaptor: get and print video information using ffmpeg library.
 * #### References ####
 *   (2018/04/15):
 *   https://github.com/mpenkov/ffmpeg-tutorial/blob/master/tutorial01.c
 *   https://github.com/mikeboers/PyAV/blob/develop/av/container/input.pyx
 *   https://www.ffmpeg.org/doxygen/3.4/index.html
 *   (2018/04/29): https://stackoverflow.com/a/12563019
 * #### Compilation command ####
 *   g++ videoraptor.cpp -o videoraptor -I <ffmpeg-include-dir> -L <ffmpeg-lib-dir> -lavcodec -lavformat -lavutil
 * #### Usage ####
 *   videoraptor <video_file_path>
 * #### Usage notes ####
 *   On Windows, you need FFMPEG DLLs: avcodec-*.dll, avformat-*.dll, avutil-*.dll, swresample-*.dll.
 *   Some errors and warnings may be written in stderr:
 *     #ERROR <error_message>
 *     #HELP <error_message>
 *     #FINISHED <integer>
 *     #IGNORED <video_filename>
 *     #LOADED <integer>
 *     #MESSAGE <message>
 *     #VIDEO_ERROR[<video_filename>]<error_message>
 *     #VIDEO_THUMBNAIL_ERROR[<video_filename>]<error_message>
 *     #VIDEO_WARNING[<video_filename>]<warning_message>
 * **/

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/log.h>
#include <libavutil/imgutils.h>
}

#include <cstring>
#include <iostream>
#include <fstream>

#define STR_FOUND(pos) (pos != std::string::npos)
#define STR_NONE std::string::npos

const char* const digits = "0123456789ABCDEF";
const std::string* currentFilenamePtr = nullptr;

void customCallback(void* avcl, int level, const char* fmt, va_list vl) {
	if (level < av_log_get_level() && currentFilenamePtr) {
		char currentMessage[2048] = {};
		vsnprintf(currentMessage, sizeof(currentMessage), fmt, vl);
		for (char& character: currentMessage) {
			if (character == '\r' || character == '\n')
				character = ' ';
		}
		std::cout << "#VIDEO_WARNING[" << *currentFilenamePtr << "]" << currentMessage << std::endl;
	}
}

inline void replaceString(std::string& member, const char* from, const char* to) {
	size_t from_len = strlen(from);
	size_t to_len = strlen(to);
	size_t pos = 0;
	do {
		pos = member.find(from, pos);
		if (pos != std::string::npos) {
			member.replace(pos, from_len, to);
			pos += to_len;
		}
	} while (pos != std::string::npos);
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

struct HexEncoder {
	const char* str;

	explicit HexEncoder(const char* inputString): str(inputString) {}
};

struct CStringToJson {
	const char* str;

	explicit CStringToJson(const char* inputString): str(inputString) {}

	explicit CStringToJson(const std::string& inputString): str(inputString.c_str()) {}
};

struct StreamInfo {
	int index; // stream index
	AVStream* stream;
	AVCodecContext* codecContext;
	AVCodec* codec;

	StreamInfo(): index(-1), stream(nullptr), codecContext(nullptr), codec(nullptr) {}
};

class VideoInfo {
	AVFormatContext* format;
	StreamInfo audioStream;
	StreamInfo videoStream;
	std::string filename;

	bool thumbnailError(const char* message) {
		std::cout << "#VIDEO_THUMBNAIL_ERROR[" << filename << "]" << message << std::endl;
		return false;
	}

	bool loadStreamInfo(StreamInfo* streamInfo) {
		streamInfo->stream = format->streams[streamInfo->index];
		streamInfo->codec = avcodec_find_decoder(streamInfo->stream->codecpar->codec_id);
		if (!streamInfo->codec)
			return error("Unable to find codec.");
		streamInfo->codecContext = avcodec_alloc_context3(streamInfo->codec);
		if (!streamInfo->codecContext)
			return error("Unable to alloc codec context.");
		if (avcodec_parameters_to_context(streamInfo->codecContext, streamInfo->stream->codecpar) < 0)
			return error("Unable to convert codec parameters to context.");
		if (avcodec_open2(streamInfo->codecContext, streamInfo->codec, NULL) < 0)
			return error("Unable to open codec.");
		return true;
	}

	bool savePNG(AVFrame* frame, const std::string& thumbnailPath) {
		AVPacket pkt;
		AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
		if (!codec)
			return thumbnailError("PNG codec not found.");
		AVCodecContext* c = avcodec_alloc_context3(codec);
		if (!c)
			return thumbnailError("Could not allocate PNG codec context.");
		c->time_base = (AVRational) {1, 25};
		c->width = videoStream.codecContext->width;
		c->height = videoStream.codecContext->height;
		c->pix_fmt = AV_PIX_FMT_RGB24;
		if (avcodec_open2(c, codec, NULL) < 0)
			return thumbnailError("Could not open PNG codec.");
		av_init_packet(&pkt);
		pkt.data = NULL;
		pkt.size = 0;
		if (avcodec_send_frame(c, frame) < 0)
			return thumbnailError("Error sending frame for PNG encoding.");
		if (avcodec_receive_packet(c, &pkt) < 0)
			return thumbnailError("Error receiving packet for PNG encoding.");
		FILE* f = fopen(thumbnailPath.c_str(), "wb");
		fwrite(pkt.data, 1, pkt.size, f);
		fclose(f);
		av_packet_unref(&pkt);
		avcodec_close(c);
		av_free(c);
		return true;
	}

public:
	VideoInfo(): format(nullptr), audioStream(), videoStream(), filename() {}
	~VideoInfo() {
		if (format) {
			if (videoStream.codecContext) {
				avcodec_close(videoStream.codecContext);
				avcodec_free_context(&videoStream.codecContext);
			}
			if (audioStream.codecContext) {
				avcodec_close(audioStream.codecContext);
				avcodec_free_context(&audioStream.codecContext);
			}
			avformat_close_input(&format);
		}
	}

	bool error(const char* message) {
		std::cout << "#VIDEO_ERROR[" << filename << "]" << message << std::endl;
		return false;
	}

	bool load(const char* videoFilename) {
		filename = videoFilename;
		currentFilenamePtr = &filename;
		// Open video file
		if (avformat_open_input(&format, videoFilename, NULL, NULL) != 0)
			return error("Unable to open file.");
		// Retrieve stream information
		if (avformat_find_stream_info(format, NULL) < 0)
			return error("Unable to get streams info.");
		// Find first video stream and audio stream
		for (int i = 0; i < format->nb_streams; ++i) {
			if (videoStream.index < 0 || audioStream.index < 0) {
				if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
					if (videoStream.index < 0)
						videoStream.index = i;
				} else if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
					if (audioStream.index < 0)
						audioStream.index = i;
				}
			}
		}
		if (videoStream.index == -1)
			return error("Unable to find a video stream.");
		if (!loadStreamInfo(&videoStream))
			return error("Unable to load video stream.");
		if (audioStream.index >= 0 && !loadStreamInfo(&audioStream))
			return error("Unable to load audio stream.");
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
			return thumbnailError("Unable to allocate input frame.");

		// Allocate an AVFrame structure
		pFrameRGB = av_frame_alloc();
		if (pFrameRGB == NULL)
			return thumbnailError("Unable to allocate output frame.");

		// Determine required buffer size and allocate buffer
		numBytes = av_image_get_buffer_size(
				AV_PIX_FMT_RGB24, videoStream.codecContext->width, videoStream.codecContext->height, align);
		buffer = (uint8_t*) av_malloc(numBytes * sizeof(uint8_t));
		if (!buffer)
			return thumbnailError("Unable to allocate output frame buffer.");

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
		if (av_seek_frame(format, -1, format->duration / 2, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY) < 0)
			return thumbnailError("Unable to seek into video.");

		// Read frames and save first video frame as image to disk
		while (!saved && av_read_frame(format, &packet) >= 0) {
			// Is this a packet from the video stream?
			if (packet.stream_index == videoStream.index) {
				// Decode video frame
				int ret = avcodec_send_packet(videoStream.codecContext, &packet);
				if (ret < 0)
					return thumbnailError("Unable to send packet for decoding.");
				ret = avcodec_receive_frame(videoStream.codecContext, pFrame);
				if (ret == AVERROR(EAGAIN))
					continue;
				if (ret == AVERROR_EOF || ret < 0)
					return thumbnailError("Error while decoding video.");
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
			return thumbnailError("Unable to save a thumbnail.");

		// Free buffer and frames
		av_packet_unref(&packet);
		av_free(buffer);
		av_frame_free(&pFrameRGB);
		av_frame_free(&pFrame);

		return true;
	}

	friend std::ostream& operator<<(std::ostream& o, const VideoInfo& videoInfo);
};

inline std::ostream& operator<<(std::ostream& o, const HexEncoder& hexEncoder) {
	const char* str = hexEncoder.str;
	o << '"';
	while (*str) {
		unsigned char c = *str;
		o << digits[c >> 4] << digits[c & (((unsigned int) 1 << 4) - 1)];
		++str;
	}
	o << '"';
	return o;
}

inline std::ostream& operator<<(std::ostream& o, const CStringToJson& cStringToJson) {
	std::string outputString(cStringToJson.str);
	replaceString(outputString, "\"", "\\\"");
	replaceString(outputString, "\\", "\\\\");
	o << '"' << outputString << '"';
	return o;
}

inline std::ostream& operator<<(std::ostream& o, const VideoInfo& videoInfo) {
	AVRational* frame_rate = &videoInfo.videoStream.stream->avg_frame_rate;
	if (!frame_rate->den)
		frame_rate = &videoInfo.videoStream.stream->r_frame_rate;
	o << '{';
	o << R"("filename":)" << CStringToJson(videoInfo.filename) << ',';
	o << R"("duration":)" << videoInfo.format->duration << ',';
	o << R"("duration_time_base":)" << AV_TIME_BASE << ',';
	o << R"("size":)" << avio_size(videoInfo.format->pb) << ',';
	o << R"("container_format":)" << CStringToJson(videoInfo.format->iformat->long_name) << ',';
	o << R"("width":)" << videoInfo.videoStream.codecContext->width << ',';
	o << R"("height":)" << videoInfo.videoStream.codecContext->height << ',';
	o << R"("video_codec":)" << CStringToJson(videoInfo.videoStream.codec->long_name) << ',';
	o << R"("frame_rate":")" << frame_rate->num << '/' << frame_rate->den << '"';
	if (videoInfo.audioStream.index >= 0) {
		o << ',';
		o << R"("audio_codec":)" << CStringToJson(videoInfo.audioStream.codec->long_name) << ',';
		o << R"("sample_rate":)" << videoInfo.audioStream.codecContext->sample_rate << ',';
		o << R"("bit_rate":)" << videoInfo.audioStream.codecContext->bit_rate;
	}
	AVDictionaryEntry* tag = av_dict_get(videoInfo.format->metadata, "title", NULL, AV_DICT_IGNORE_SUFFIX);
	if (tag)
		o << ',' << R"("title":)" << HexEncoder(tag->value);
	o << '}';
	return o;
};

inline bool run(const char* videoFilename, const char* thumbFolder = nullptr, const char* thumbName = nullptr) {
	VideoInfo videoInfo;
	if (videoInfo.load(videoFilename)) {
		if (thumbFolder) {
			if (!thumbName)
				return videoInfo.error("Cannot generate thumbnail without thumbnail name.");
			std::string thumbnailPath = thumbFolder;
			if (!thumbnailPath.empty()) {
				char lastChar = thumbnailPath[thumbnailPath.size() - 1];
				if (lastChar != '/' && lastChar != '\\')
					return videoInfo.error("Thumbnail folder does not end with an OS path separator.");
			}
			thumbnailPath += thumbName;
			thumbnailPath += ".png";
			// Generate thumbnail.
			videoInfo.generateThumbnail(thumbnailPath);
		} else {
			// Print video info.
			std::cout << videoInfo << std::endl;
		}
	}
	return true;
}

int main(int argc, char* argv[]) {
	// Check if there are arguments.
	if (argc != 2 && argc != 4) {
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
		std::cout << "#USAGE     Parse given text file. Each line must be a single video command "
			   "with arguments separated by a tab character." << std::endl;
		return -1;
	}
	// Register all formats and codecs
	av_register_all();
	av_log_set_callback(customCallback);
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
				size_t posTab2 = STR_FOUND(posTab1) ? line.find('\t', posTab1 + 1) : STR_NONE;
				const char* videoFilename = line.c_str();
				const char* thumbnailFolder = nullptr;
				const char* thumbnailName = nullptr;
				if (STR_FOUND(posTab1)) {
					if (posTab2 == STR_NONE) {
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
