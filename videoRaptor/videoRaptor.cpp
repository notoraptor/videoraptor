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
 *   g++ videoraptor.cpp lodepng.h lodepng.cpp -o .local\videoraptor -I . -I ..\ffmpeg-4.0-win64-dev\include -L ..\ffmpeg-4.0-win64-dev\lib -lavcodec -lavformat -lavutil -lswscale -O3
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

#include <cstddef>
#include <cstring>
#include <iostream>
#include "videoRaptorBatch/videoRaptorBatch.hpp"
#include "videoRaptorOnce/videoRaptorOnce.hpp"

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
	// Parse arguments.
	if (extensionIsTxt(argv[1])) {
		if (!videoRaptorTxtFile(argv[1], nullptr))
			return -1;
	} else {
		const char* videoFilename = argv[1];
		const char* thumbnailFolder = nullptr;
		const char* thumbnailName = nullptr;
		if (argc == 4) {
			thumbnailFolder = argv[2];
			thumbnailName = argv[3];
		}
		if (!videoRaptorOnce(videoFilename, thumbnailFolder, thumbnailName, nullptr))
			return -1;
	}
	return 0;
}
