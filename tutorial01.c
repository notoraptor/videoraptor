// tutorial01.c
//
// This tutorial was written by Stephen Dranger (dranger@gmail.com).
//
// Code based on a tutorial by Martin Bohme (boehme@inb.uni-luebeckREMOVETHIS.de)
// Tested on Gentoo, CVS version 5/01/07 compiled with GCC 4.1.1

// A small sample program that shows how to use libavformat and libavcodec to
// read video from a file.
//
// Use the Makefile to build all examples.
//
// Run using
//
// tutorial01 myvideofile.mpg
//
// to write the first five frames from "myvideofile.mpg" to disk in PPM
// format.

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#include <stdio.h>

int savePNG(AVFrame* frame, int width, int height) {
	AVPacket pkt;
	AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
	if (!codec) {
		printf("Codec not found\n");
		return 0;
	}
	AVCodecContext* c = avcodec_alloc_context3(codec);
	if (!c) {
		printf("Could not allocate video codec context\n");
		return 0;
	}
	c->time_base = (AVRational) {1, 25};
	c->width = width;
	c->height = height;
	c->pix_fmt = AV_PIX_FMT_RGB24;
	if (avcodec_open2(c, codec, NULL) < 0) {
		printf("Could not open codec\n");
		return 0;
	}
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;
	if (avcodec_send_frame(c, frame) < 0) {
		printf("Error sending frame\n");
		return 0;
	}
	if (avcodec_receive_packet(c, &pkt) < 0) {
		printf("Error receiving packet\n");
		return 0;
	}
	FILE* f = fopen("x.png", "wb");
	fwrite(pkt.data, 1, pkt.size, f);
	av_packet_unref(&pkt);
	avcodec_close(c);
	av_free(c);
	return 1;
}

int generateThumbnail(AVFormatContext* pFormatCtx, AVCodecContext* pCodecCtx, int videoStreamIndex) {
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
		return -1;

	// Allocate an AVFrame structure
	pFrameRGB = av_frame_alloc();
	if (pFrameRGB == NULL)
		return -2;

	// Determine required buffer size and allocate buffer
	numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, align);
	buffer = (uint8_t*) av_malloc(numBytes * sizeof(uint8_t));
	if (!buffer)
		return -3;

	sws_ctx = sws_getContext(
			pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
			pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24,
			SWS_BILINEAR, NULL, NULL, NULL
	);

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer,
						 AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, align);

	// seek
	if (av_seek_frame(pFormatCtx, -1, pFormatCtx->duration / 2, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY) < 0)
		return -4;

	// Read frames and save first video frame as image to disk
	while (!saved && av_read_frame(pFormatCtx, &packet) >= 0) {
		// Is this a packet from the video stream?
		if (packet.stream_index == videoStreamIndex) {
			// Decode video frame
			int ret = avcodec_send_packet(pCodecCtx, &packet);
			if (ret < 0)
				return -5;
			ret = avcodec_receive_frame(pCodecCtx, pFrame);
			if (ret == AVERROR(EAGAIN))
				continue;
			if (ret == AVERROR_EOF || ret < 0)
				return -6;
			// Convert the image from its native format to RGB
			sws_scale(sws_ctx, (uint8_t const* const*) pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
					  pFrameRGB->data, pFrameRGB->linesize);
			// Save the frame to disk
			pFrameRGB->width = pCodecCtx->width;
			pFrameRGB->height = pCodecCtx->height;
			pFrameRGB->format = AV_PIX_FMT_RGB24;
			if (!savePNG(pFrameRGB, pCodecCtx->width, pCodecCtx->height))
				return -7;
			saved = 1;
		}
	}

	if (!saved)
		return -8;

	// Free buffer and frames
	av_packet_unref(&packet);
	av_free(buffer);
	av_frame_free(&pFrameRGB);
	av_frame_free(&pFrame);

	return 0;
}

int main(int argc, char* argv[]) {
	AVFormatContext* pFormatCtx = NULL;
	AVCodecContext* pCodecCtx = NULL;
	AVCodec* pCodec = NULL;

	int videoStream;


	if (argc < 2) {
		printf("Please provide a movie file\n");
		return -1;
	}
	// Register all formats and codecs
	av_register_all();

	// Open video file
	if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0)
		return -1; // Couldn't open file

	// Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
		return -1; // Couldn't find stream information

	// Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, argv[1], 0);

	// Find the first video stream
	videoStream = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	if (videoStream == -1)
		return -1; // Didn't find a video stream

	// Find the decoder for the video stream
	pCodec = avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);
	if (pCodec == NULL) {
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}

	// Get a pointer to the codec context for the video stream
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (pCodecCtx == NULL) {
		fprintf(stderr, "Unable to alloc codec context.");
		return -1;
	}
	if (avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar) < 0) {
		fprintf(stderr, "Unable to convert codecpar to codec context.");
		return -1;
	}

	// Open codec
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
		return -1; // Could not open codec

	// TODO New code starts here.
	int ret = generateThumbnail(pFormatCtx, pCodecCtx, videoStream);
	if (ret != 0) {
		fprintf(stderr, "Error %d\n", ret);
		return ret;
	}

	// Close codec and video file
	avcodec_close(pCodecCtx);
	avcodec_free_context(&pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}
