//
// Created by notoraptor on 23/07/2018.
//

#ifndef VIDEORAPTORBATCH_THUMBNAIL_CONTEXT_HPP
#define VIDEORAPTORBATCH_THUMBNAIL_CONTEXT_HPP

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
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

	ThumbnailContext(): tmpFrame(nullptr), frame(nullptr), swFrame(nullptr), frameRGB(nullptr), buffer(nullptr),
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

#endif //VIDEORAPTORBATCH_THUMBNAIL_CONTEXT_HPP
