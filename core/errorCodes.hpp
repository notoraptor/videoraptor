//
// Created by notoraptor on 09/03/2019.
//

#ifndef VIDEORAPTOR_ERRORCODES_HPP
#define VIDEORAPTOR_ERRORCODES_HPP

#include <iostream>

enum VideoRaptorErrors {
	SUCCESS_NOTHING						= 0b00000000000000000000000000000000,	// Nothing happened.
	ERROR_PNG_CODEC						= 0b00000000000000000000000000000001,	// global	// PNG codec not found.
	ERROR_NO_BATCH_GIVEN				= 0b00000000000000000000000000000010,	// global	// No batch given.
	ERROR_NO_STREAM_INFO				= 0b00000000000000000000000000000100,	// Unable to get streams info.
	WARNING_OPEN_ASCII_FILENAME			= 0b00000000000000000000000000001000,	// Error opening file, retrying with unicode filename handling.
	ERROR_OPEN_FILE						= 0b00000000000000000000000000010000,	// Unable to open file.
	ERROR_FIND_VIDEO_STREAM				= 0b00000000000000000000000000100000,	// Unable to find a video stream.
	ERROR_ALLOC_CODEC_CONTEXT			= 0b00000000000000000000000001000000,	// Unable to alloc codec context.
	ERROR_CONVERT_CODEC_PARAMS			= 0b00000000000000000000000010000000,	// Unable to convert codec parameters to context.
	ERROR_OPEN_CODEC					= 0b00000000000000000000000100000000,	// Unable to open codec.
	ERROR_INVALID_PIX_FMT				= 0b00000000000000000000001000000000,	// Video stream has invalid pixel format.
	ERROR_INVALID_WIDTH					= 0b00000000000000000000010000000000,	// Video stream has invalid width.
	ERROR_INVALID_HEIGHT				= 0b00000000000000000000100000000000,	// Video stream has invalid height.
	WARNING_FIND_DEVICE_CONFIG			= 0b00000000000000000001000000000000,	// Unable to find HW device config to decode this file.
	WARNING_CREATE_DEVICE_CONFIG		= 0b00000000000000000010000000000000,	// Failed to create any HW device.
	ERROR_HW_SURFACE_FORMAT				= 0b00000000000000000100000000000000,	// Failed to get HW surface format.
	ERROR_SEEK_VIDEO					= 0b00000000000000001000000000000000,	// Unable to seek into video.
	ERROR_SEND_PACKET					= 0b00000000000000010000000000000000,	// Unable to send packet for decoding.
	ERROR_ALLOC_INPUT_FRAME				= 0b00000000000000100000000000000000,	// Unable to allocate input frame.
	ERROR_DECODE_VIDEO					= 0b00000000000001000000000000000000,	// Error while decoding video.
	ERROR_ALLOC_HW_INPUT_FRAME			= 0b00000000000010000000000000000000,	// Unable to allocate HW input frame.
	ERROR_HW_DATA_TRANSFER				= 0b00000000000100000000000000000000, 	// Error transferring the data to system memory.
	ERROR_ALLOC_OUTPUT_FRAME			= 0b00000000001000000000000000000000,	// Unable to allocate output frame.
	ERROR_ALLOC_OUTPUT_FRAME_BUFFER		= 0b00000000010000000000000000000000,	// Unable to allocate output frame buffer.
	ERROR_SAVE_THUMBNAIL				= 0b00000000100000000000000000000000,	// Unable to save a thumbnail.
	ERROR_PNG_ENCODER					= 0b00000001000000000000000000000000,	// Error while encoding to PNG.
	WARNING_NO_DEVICE_CODEC				= 0b00000010000000000000000000000000,	// Opened without device codec.
	ERROR_CUSTOM_FORMAT_CONTEXT			= 0b00000100000000000000000000000000,	// Error while building custom format context.
	ERROR_CUSTOM_FORMAT_CONTEXT_OPEN	= 0b00001000000000000000000000000000,	// Error while opening custom format context.
	SUCCESS_DONE						= 0b00010000000000000000000000000000,	// Work done.
	ERROR_CODE_000000030				= 0b00100000000000000000000000000000,
	ERROR_CODE_000000031				= 0b01000000000000000000000000000000,
	ERROR_CODE_000000032				= 0b10000000000000000000000000000000,
};

inline void printError(unsigned int error) {
	if (error & SUCCESS_NOTHING) std::cout << "SUCCESS_NOTHING" << std::endl;
	if (error & ERROR_PNG_CODEC) std::cout << "ERROR_PNG_CODEC" << std::endl;
	if (error & ERROR_NO_BATCH_GIVEN) std::cout << "ERROR_NO_BATCH_GIVEN" << std::endl;
	if (error & ERROR_NO_STREAM_INFO) std::cout << "ERROR_NO_STREAM_INFO" << std::endl;
	if (error & WARNING_OPEN_ASCII_FILENAME) std::cout << "WARNING_OPEN_ASCII_FILENAME" << std::endl;
	if (error & ERROR_OPEN_FILE) std::cout << "ERROR_OPEN_FILE" << std::endl;
	if (error & ERROR_FIND_VIDEO_STREAM) std::cout << "ERROR_FIND_VIDEO_STREAM" << std::endl;
	if (error & ERROR_ALLOC_CODEC_CONTEXT) std::cout << "ERROR_ALLOC_CODEC_CONTEXT" << std::endl;
	if (error & ERROR_CONVERT_CODEC_PARAMS) std::cout << "ERROR_CONVERT_CODEC_PARAMS" << std::endl;
	if (error & ERROR_OPEN_CODEC) std::cout << "ERROR_OPEN_CODEC" << std::endl;
	if (error & ERROR_INVALID_PIX_FMT) std::cout << "ERROR_INVALID_PIX_FMT" << std::endl;
	if (error & ERROR_INVALID_WIDTH) std::cout << "ERROR_INVALID_WIDTH" << std::endl;
	if (error & ERROR_INVALID_HEIGHT) std::cout << "ERROR_INVALID_HEIGHT" << std::endl;
	if (error & WARNING_FIND_DEVICE_CONFIG) std::cout << "WARNING_FIND_DEVICE_CONFIG" << std::endl;
	if (error & WARNING_CREATE_DEVICE_CONFIG) std::cout << "WARNING_CREATE_DEVICE_CONFIG" << std::endl;
	if (error & ERROR_HW_SURFACE_FORMAT) std::cout << "ERROR_HW_SURFACE_FORMAT" << std::endl;
	if (error & ERROR_SEEK_VIDEO) std::cout << "ERROR_SEEK_VIDEO" << std::endl;
	if (error & ERROR_SEND_PACKET) std::cout << "ERROR_SEND_PACKET" << std::endl;
	if (error & ERROR_ALLOC_INPUT_FRAME) std::cout << "ERROR_ALLOC_INPUT_FRAME" << std::endl;
	if (error & ERROR_DECODE_VIDEO) std::cout << "ERROR_DECODE_VIDEO" << std::endl;
	if (error & ERROR_ALLOC_HW_INPUT_FRAME) std::cout << "ERROR_ALLOC_HW_INPUT_FRAME" << std::endl;
	if (error & ERROR_HW_DATA_TRANSFER) std::cout << "ERROR_HW_DATA_TRANSFER" << std::endl;
	if (error & ERROR_ALLOC_OUTPUT_FRAME) std::cout << "ERROR_ALLOC_OUTPUT_FRAME" << std::endl;
	if (error & ERROR_ALLOC_OUTPUT_FRAME_BUFFER) std::cout << "ERROR_ALLOC_OUTPUT_FRAME_BUFFER" << std::endl;
	if (error & ERROR_SAVE_THUMBNAIL) std::cout << "ERROR_SAVE_THUMBNAIL" << std::endl;
	if (error & ERROR_PNG_ENCODER) std::cout << "ERROR_PNG_ENCODER" << std::endl;
	if (error & WARNING_NO_DEVICE_CODEC) std::cout << "WARNING_NO_DEVICE_CODEC" << std::endl;
	if (error & ERROR_CUSTOM_FORMAT_CONTEXT) std::cout << "ERROR_CUSTOM_FORMAT_CONTEXT" << std::endl;
	if (error & ERROR_CUSTOM_FORMAT_CONTEXT_OPEN) std::cout << "ERROR_CUSTOM_FORMAT_CONTEXT_OPEN" << std::endl;
	if (error & SUCCESS_DONE) std::cout << "SUCCESS_DONE" << std::endl;
	if (error & ERROR_CODE_000000030) std::cout << "ERROR_CODE_000000030" << std::endl;
	if (error & ERROR_CODE_000000031) std::cout << "ERROR_CODE_000000031" << std::endl;
	if (error & ERROR_CODE_000000032) std::cout << "ERROR_CODE_000000032" << std::endl;
}

#endif //VIDEORAPTOR_ERRORCODES_HPP
