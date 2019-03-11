//
// Created by notoraptor on 28/07/2018.
//

#ifndef VIDEORAPTOR_COMPAT_WIN_HPP
#define VIDEORAPTOR_COMPAT_WIN_HPP
#ifdef WIN32

extern "C" {
#include <libavformat/avformat.h>
};
#include <sys/stat.h>
#include <cstdint>
#include <ostream>
#include <vector>
#include "FileHandle.hpp"
#include "unicode.hpp"
#include "VideoDetails.hpp"
#include "errorCodes.hpp"

inline bool fileSize(const char* filename, size_t* out) {
	struct __stat64 buf;
	int result = _stat64( filename, &buf );
	if (result == 0)
		*out = buf.st_size;
	return result == 0;
}

inline bool fileSize(const wchar_t* filename, size_t* out) {
	struct __stat64 buf;
	int result = _wstat64( filename, &buf );
	if (result == 0)
		*out = buf.st_size;
	return result == 0;
}

inline int readFromFile(void* opaque, uint8_t* buffer, int buffer_size) {
	size_t count_read = fread(buffer, 1, buffer_size, ((FileHandle*)opaque)->file);
	if (!count_read)
		return AVERROR_EOF;
	return count_read;
}

inline int64_t seekInFile(void* opaque, int64_t offset, int whence) {
	FileHandle* fileHandle = (FileHandle*) opaque;
	if (whence == AVSEEK_SIZE) {
		size_t size = 0;
		bool ret = !fileHandle->unicodeFilename.empty() ? fileSize(fileHandle->unicodeFilename.data(), &size) : fileSize(fileHandle->filename, &size);
		return ret ? (int64_t)size : -1;
	}
	int ret = _fseeki64(fileHandle->file, offset, whence);
	return ret < 0 ? ret : _ftelli64(fileHandle->file);
}

struct ProbeBuffer {
	uint8_t* probe_buffer;
	ProbeBuffer(): probe_buffer(nullptr) {}
	~ProbeBuffer() {
		if (probe_buffer)
			av_freep(&probe_buffer);
	}
};

#define OPEN_ERROR_PROBE_BUFFER_MEMORY 		1	// Memory error for probe buffer.
#define OPEN_ERROR_READ_PROBE_BYTES 		2	// Error while reading probe bytes from file.
#define OPEN_ERROR_RESET_CURSOR 			3	// Error while resetting file cursor to beginning.
#define OPEN_ERROR_INPUT_FORMAT 			4	// Error while getting input format.
#define OPEN_ERROR_AVIO_BUFFER_MEMORY 		5	// Memory error for AVIO context buffer.
#define OPEN_ERROR_AVIO_INIT 				6	// Memory error for AVIO context initialization.
#define OPEN_ERROR_FORMAT_CONTEXT_MEMORY 	7	// Error while allocating format context.

inline bool customFormatContextError(VideoErrors* videoErrors, unsigned int localErrorCode) {
	const char* const digits = "0123456789";
	char errorDetail[2] = {digits[localErrorCode], '\0'};
	return videoRaptorError(videoErrors, ERROR_CUSTOM_FORMAT_CONTEXT, errorDetail);
}

inline bool openCustomFormatContext(FileHandle& fileHandle, AVFormatContext** format, AVIOContext** avioContext, VideoErrors* videoErrors) {
	int ret = 0;
	std::string errorString;
	size_t avio_ctx_buffer_size = 4096;
	size_t probe_buffer_size = avio_ctx_buffer_size + AVPROBE_PADDING_SIZE;
	size_t n_bytes_read = 0;
	uint8_t* avio_ctx_buffer = nullptr;
	AVProbeData probeData;
	AVInputFormat* inputFormat = nullptr;
	ProbeBuffer probeBuffer;

	fileHandle.file = fopen(fileHandle.filename, "rb");
	if (!fileHandle.file) {
		videoRaptorError(videoErrors, WARNING_OPEN_ASCII_FILENAME);
		// To handle long file names, we assume file name is an absolute path, and we add prefix \\?\.
		// See (2018/07/29): https://docs.microsoft.com/fr-fr/windows/desktop/FileIO/naming-a-file#maximum-path-length-limitation
		fileHandle.unicodeFilename.push_back('\\');
		fileHandle.unicodeFilename.push_back('\\');
		fileHandle.unicodeFilename.push_back('?');
		fileHandle.unicodeFilename.push_back('\\');
		unicode_convert(fileHandle.filename, fileHandle.unicodeFilename);
		fileHandle.unicodeFilename.push_back('\0');
		fileHandle.file = _wfopen(fileHandle.unicodeFilename.data(), L"rb");
	}
	if (!fileHandle.file)
		return videoRaptorError(videoErrors, ERROR_OPEN_FILE);

	// Get input format.
	probeBuffer.probe_buffer = (uint8_t*) av_malloc(probe_buffer_size);
	if (!probeBuffer.probe_buffer)
		return customFormatContextError(videoErrors, OPEN_ERROR_PROBE_BUFFER_MEMORY);

	memset(probeBuffer.probe_buffer, 0, probe_buffer_size);
	n_bytes_read = fread(probeBuffer.probe_buffer, 1, avio_ctx_buffer_size, fileHandle.file);
	if (n_bytes_read != avio_ctx_buffer_size && ferror(fileHandle.file))
		return customFormatContextError(videoErrors, OPEN_ERROR_READ_PROBE_BYTES);

	if (fseek(fileHandle.file, 0, SEEK_SET) != 0)
		return customFormatContextError(videoErrors, OPEN_ERROR_RESET_CURSOR);

	probeData.buf = probeBuffer.probe_buffer;
	probeData.buf_size = n_bytes_read;
	probeData.filename = "";
	probeData.mime_type = nullptr;
	inputFormat = av_probe_input_format(&probeData, 1);
	if (!inputFormat)
		return customFormatContextError(videoErrors, OPEN_ERROR_INPUT_FORMAT);

	// Create input format context.
	avio_ctx_buffer = (uint8_t*) av_malloc(avio_ctx_buffer_size);
	if (!avio_ctx_buffer)
		return customFormatContextError(videoErrors, OPEN_ERROR_AVIO_BUFFER_MEMORY);

	if (!(*avioContext = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0, &fileHandle, readFromFile, NULL, seekInFile)))
		return customFormatContextError(videoErrors, OPEN_ERROR_AVIO_INIT);

	// Open format context.
	if (!(*format = avformat_alloc_context()))
		return customFormatContextError(videoErrors, OPEN_ERROR_FORMAT_CONTEXT_MEMORY);

	(*format)->pb = *avioContext;
	(*format)->iformat = inputFormat;
	(*format)->flags = AVFMT_FLAG_CUSTOM_IO;

	if ((ret = avformat_open_input(format, NULL, NULL, NULL)) != 0) {
		char err_buf[AV_ERROR_MAX_STRING_SIZE];
		av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, ret);
		return videoRaptorError(videoErrors, ERROR_CUSTOM_FORMAT_CONTEXT_OPEN, err_buf);
	};
	return true;
}

#endif
#endif //VIDEORAPTOR_COMPAT_WIN_HPP
