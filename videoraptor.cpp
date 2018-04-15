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
 * **/

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <algorithm>

using namespace std;

void replaceString(std::string &member, const char *from, const char *to) {
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

void stripString(std::string &s) {
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

std::string encodeHex(const std::string &s) {
	static const char *const digits = "0123456789ABCDEF";
	size_t n_bytes = s.length();
	std::string out(2 * n_bytes, '0');
	for (size_t i = 0; i < n_bytes; ++i) {
		char c = s[i];
		out[2 * i] = digits[c >> 4];
		out[2 * i + 1] = digits[c & (((unsigned int) 1 << 4) - 1)];
	}
	return out;
}

std::string jsonString(const char *inputString) {
	std::string forJson(inputString);
	replaceString(forJson, "\"", "\\\"");
	replaceString(forJson, "\\", "\\\\");
	return forJson;
}

bool extensionIsTxt(const char *s) {
	size_t len = strlen(s);
	return len >= 4 && s[len - 4] == '.' && (s[len - 3] == 't' || s[len - 3] == 'T')
		   && (s[len - 2] == 'x' || s[len - 2] == 'X') && (s[len - 1] == 't' || s[len - 1] == 'T');
}

struct StreamInfo {
	AVStream *stream;
	AVCodecContext *codecContext;
	AVCodec *codec;

	StreamInfo() : stream(nullptr), codecContext(nullptr), codec(nullptr) {}
};

struct VideoInfo {
	std::string filename;
	AVFormatContext *videoFormat;
	StreamInfo audioStream;
	StreamInfo videoStream;
	int audioStreamIndex;
	int videoStreamIndex;

	bool error(const char *message) {
		cout << "##ERROR[" << filename << "] " << message << endl;
		return false;
	}

	bool getStreamInfo(int streamIndex, StreamInfo *streamInfo) {
		streamInfo->stream = videoFormat->streams[streamIndex];
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
	VideoInfo() : filename(), videoFormat(nullptr), audioStream(), videoStream(),
				  audioStreamIndex(-1), videoStreamIndex(-1) {}

	~VideoInfo() {
		if (videoFormat) {
			if (videoStream.codecContext) {
				avcodec_close(videoStream.codecContext);
				avcodec_free_context(&videoStream.codecContext);
			}
			if (audioStream.codecContext) {
				avcodec_close(audioStream.codecContext);
				avcodec_free_context(&audioStream.codecContext);
			}
			avformat_close_input(&videoFormat);
		}
	}

	bool load(const char *videoFilename) {
		filename = videoFilename;
		// Open video file
		if (avformat_open_input(&videoFormat, videoFilename, NULL, NULL) != 0)
			return error("Unable to open file.");
		// Retrieve stream information
		if (avformat_find_stream_info(videoFormat, NULL) < 0)
			return error("Unable to get streams info.");
		// Find first video stream and audio stream
		for (int i = 0; i < videoFormat->nb_streams; ++i) {
			if (videoStreamIndex < 0 || audioStreamIndex < 0) {
				if (videoFormat->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
					if (videoStreamIndex < 0)
						videoStreamIndex = i;
				} else if (videoFormat->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
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

	void print() {
		/** Print information. **/
		AVRational frame_rate = videoStream.stream->avg_frame_rate;
		cout << '@' << filename << endl;
		cout << "duration\t" << videoFormat->duration << endl;
		cout << "duration_time_base\t" << AV_TIME_BASE << endl;
		cout << "size\t" << avio_size(videoFormat->pb) << endl;
		cout << "container_format\t" << videoFormat->iformat->long_name << endl;
		cout << "width\t" << videoStream.codecContext->width << endl;
		cout << "height\t" << videoStream.codecContext->height << endl;
		cout << "video_codec\t" << videoStream.codec->long_name << endl;
		cout << "frame_rate\t" << frame_rate.num << '/' << frame_rate.den << endl;
		if (audioStreamIndex >= 0) {
			cout << "audio_codec\t" << audioStream.codec->long_name << endl;
			cout << "sample_rate\t" << audioStream.codecContext->sample_rate << endl;
			cout << "bit_rate\t" << audioStream.codecContext->bit_rate << endl;
		}
		AVDictionaryEntry *tag = av_dict_get(videoFormat->metadata, "title", NULL, AV_DICT_IGNORE_SUFFIX);
		if (tag) {
			cout << "title\t" << tag->value << endl;
		}
	}

	void printJSON(bool first = false) {
		/** Print information. **/
		AVRational frame_rate = videoStream.stream->avg_frame_rate;
		if (!first)
			cout << ',';
		cout << '{' << endl;
		cout << R"("filename":")" << jsonString(filename.c_str()) << '"' << ',' << endl;
		cout << R"("duration":)" << videoFormat->duration << ',' << endl;
		cout << R"("duration_time_base":)" << AV_TIME_BASE << ',' << endl;
		cout << R"("size":)" << avio_size(videoFormat->pb) << ',' << endl;
		cout << R"("container_format":")" << jsonString(videoFormat->iformat->long_name) << '"' << ',' << endl;
		cout << R"("width":)" << videoStream.codecContext->width << ',' << endl;
		cout << R"("height":)" << videoStream.codecContext->height << ',' << endl;
		cout << R"("video_codec":")" << jsonString(videoStream.codec->long_name) << '"' << ',' << endl;
		cout << R"("frame_rate":")" << frame_rate.num << '/' << frame_rate.den << '"';
		if (audioStreamIndex >= 0) {
			cout << ',' << endl;
			cout << R"("audio_codec":")" << jsonString(audioStream.codec->long_name) << '"' << ',' << endl;
			cout << R"("sample_rate":)" << audioStream.codecContext->sample_rate << ',' << endl;
			cout << R"("bit_rate":)" << audioStream.codecContext->bit_rate;
		}
		AVDictionaryEntry *tag = av_dict_get(videoFormat->metadata, "title", NULL, AV_DICT_IGNORE_SUFFIX);
		if (tag) {
			cout << ',' << endl << R"("title":")" << encodeHex(std::string(tag->value)) << '"';
		}
		cout << '}';
	}
};

int main(int argc, char *argv[]) {

	if (argc < 2) {
		cerr << "##ERROR Please provide a movie file path or a TXT file containing a list of movie file paths.";
		return -1;
	}

	// Register all formats and codecs
	av_register_all();

	if (extensionIsTxt(argv[1])) {
		cerr << "# Given a TXT file." << endl;
		std::ifstream textFile(argv[1]);
		std::string line;
		bool first = true;
		cout << '[';
		while (std::getline(textFile, line)) {
			stripString(line);
			if (line.length() && line[0] != '#') {
				VideoInfo videoInfo;
				if (videoInfo.load(line.c_str())) {
					videoInfo.printJSON(first);
					first = false;
				}
			}
		}
		cout << ']' << endl;
	} else {
		cerr << "# Given 1 movie file path." << endl;
		VideoInfo videoInfo;
		if (videoInfo.load(argv[1]))
			videoInfo.printJSON();
	}

	return 0;
}
