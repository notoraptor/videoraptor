/** VideoRaptor: get and print video information using ffmpeg library.
 * ===========
 * References:
 * ===========
 * (2018/04/15):
 * https://github.com/mpenkov/ffmpeg-tutorial/blob/master/tutorial01.c
 * https://github.com/mikeboers/PyAV/blob/develop/av/container/input.pyx
 * https://www.ffmpeg.org/doxygen/3.4/index.html
 * ====================
 * Compilation command:
 * ====================
 * g++ videoraptor.cpp -o videoraptor -I <ffmpeg-include-dir> -L <ffmpeg-lib-dir> -lavcodec -lavformat -lavutil
 * ======
 * Usage:
 * ======
 * videoraptor <video_file_path>
 * ============
 * Usage notes:
 * ============
 * On Windows, you need FFMPEG DLLs: avcodec-*.dll, avformat-*.dll, avutil-*.dll, swresample-*.dll.
 * Some errors and warnings may be written in stderr:
 * #ERROR <error_message>
 * #FINISHED <integer>
 * #IGNORED <video_filename>
 * #LOADED <integer>
 * #MESSAGE <message>
 * #VIDEO_ERROR[<video_filename>]<error_message>
 * #VIDEO_WARNING[<video_filename>]<warning_message>
 * **/

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/log.h>
}

#include <cstring>
#include <iostream>
#include <fstream>

using namespace std;

const char* const digits = "0123456789ABCDEF";

const std::string* currentFilenamePtr = nullptr;
char currentMessage[2048] = {};

void custom_callback(void* avcl, int level, const char* fmt, va_list vl) {
	if (level < av_log_get_level() && currentFilenamePtr) {
		vsnprintf(currentMessage, sizeof(currentMessage), fmt, vl);
		cerr << "#VIDEO_WARNING[" << *currentFilenamePtr << "]" << currentMessage << endl;
	}
	// av_log_default_callback(avcl, level, fmt, vl);
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

inline ostream& operator<<(ostream& o, const HexEncoder& hexEncoder) {
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

struct CStringToJson {
	const char* str;

	explicit CStringToJson(const char* inputString): str(inputString) {}

	explicit CStringToJson(const string& inputString): str(inputString.c_str()) {}
};

inline ostream& operator<<(ostream& o, const CStringToJson& cStringToJson) {
	std::string outputString(cStringToJson.str);
	replaceString(outputString, "\"", "\\\"");
	replaceString(outputString, "\\", "\\\\");
	o << '"' << outputString << '"';
	return o;
}

struct StreamInfo {
	AVStream* stream;
	AVCodecContext* codecContext;
	AVCodec* codec;

	StreamInfo(): stream(nullptr), codecContext(nullptr), codec(nullptr) {}
};

struct VideoInfo {
	std::string filename;
	AVFormatContext* format;
	StreamInfo audioStream;
	StreamInfo videoStream;
	int audioStreamIndex;
	int videoStreamIndex;

	bool error(const char* message) {
		cerr << "#VIDEO_ERROR[" << filename << "]" << message << endl;
		return false;
	}

	bool getStreamInfo(int streamIndex, StreamInfo* streamInfo) {
		streamInfo->stream = format->streams[streamIndex];
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

public:
	VideoInfo(): filename(), format(nullptr), audioStream(), videoStream(),
				 audioStreamIndex(-1), videoStreamIndex(-1) {}

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
			if (videoStreamIndex < 0 || audioStreamIndex < 0) {
				if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
					if (videoStreamIndex < 0)
						videoStreamIndex = i;
				} else if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
					if (audioStreamIndex < 0)
						audioStreamIndex = i;
				}
			}
		}
		if (videoStreamIndex == -1)
			return error("Unable to find a video stream.");
		if (!getStreamInfo(videoStreamIndex, &videoStream))
			return error("Unable to load video stream.");
		if (audioStreamIndex >= 0 && !getStreamInfo(audioStreamIndex, &audioStream))
			return error("Unable to load audio stream.");
		return true;
	}

	void printJSON(bool isNext = true) {
		/** Print information. **/
		AVRational* frame_rate = &videoStream.stream->avg_frame_rate;
		if (!frame_rate->den)
			frame_rate = &videoStream.stream->r_frame_rate;
		if (isNext)
			cout << ',' << endl;
		cout << '{';
		cout << R"("filename":)" << CStringToJson(filename) << ',' << endl;
		cout << R"("duration":)" << format->duration << ',' << endl;
		cout << R"("duration_time_base":)" << AV_TIME_BASE << ',' << endl;
		cout << R"("size":)" << avio_size(format->pb) << ',' << endl;
		cout << R"("container_format":)" << CStringToJson(format->iformat->long_name) << ',' << endl;
		cout << R"("width":)" << videoStream.codecContext->width << ',' << endl;
		cout << R"("height":)" << videoStream.codecContext->height << ',' << endl;
		cout << R"("video_codec":)" << CStringToJson(videoStream.codec->long_name) << ',' << endl;
		cout << R"("frame_rate":")" << frame_rate->num << '/' << frame_rate->den << '"';
		if (audioStreamIndex >= 0) {
			cout << ',' << endl;
			cout << R"("audio_codec":)" << CStringToJson(audioStream.codec->long_name) << ',' << endl;
			cout << R"("sample_rate":)" << audioStream.codecContext->sample_rate << ',' << endl;
			cout << R"("bit_rate":)" << audioStream.codecContext->bit_rate;
		}
		AVDictionaryEntry* tag = av_dict_get(format->metadata, "title", NULL, AV_DICT_IGNORE_SUFFIX);
		if (tag) {
			cout << ',' << endl << R"("title":)" << HexEncoder(tag->value);
		}
		cout << '}';
	}
};

int main(int argc, char* argv[]) {
	// Check if there are arguments.
	if (argc < 2) {
		cerr << "#ERROR Please provide a movie file path or a TXT file containing a list of movie file paths.";
		return -1;
	}
	// Register all formats and codecs
	av_register_all();
	av_log_set_callback(custom_callback);
	// Parse arguments.
	if (extensionIsTxt(argv[1])) {
		cerr << "#MESSAGE Given a TXT file." << endl;
		std::ifstream textFile(argv[1]);
		std::string line;
		size_t count = 0;
		cout << '[';
		while (std::getline(textFile, line)) {
			stripString(line);
			if (line.length() && line[0] != '#') {
				VideoInfo videoInfo;
				if (videoInfo.load(line.c_str())) {
					videoInfo.printJSON((bool) count);
					++count;
					if (count % 25 == 0)
						cerr << "#LOADED " << count << endl;
				} else {
					cerr << "#IGNORED " << line << endl;
				}
			}
		}
		cout << ']' << endl;
		cerr << "#FINISHED " << count << endl;
	} else {
		cerr << "#MESSAGE Given 1 movie file path." << endl;
		VideoInfo videoInfo;
		if (videoInfo.load(argv[1]))
			videoInfo.printJSON(false);
	}

	return 0;
}
