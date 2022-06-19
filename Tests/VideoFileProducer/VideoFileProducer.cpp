#include "VideoFileProducer.h"

#include <Bitmap.h>
#include <MediaFile.h>
#include <MediaTrack.h>


static BMediaTrack *GetVideoTrack(BMediaFile &mediaFile, media_format &mf)
{
	for (int32 i = 0; i < mediaFile.CountTracks(); i++) {
		BMediaTrack *track = mediaFile.TrackAt(i);

		if (track->EncodedFormat(&mf) >= B_OK) {
			switch (mf.type) {
			case B_MEDIA_RAW_VIDEO:
			case B_MEDIA_ENCODED_VIDEO:
				return track;
			default:
				;
			}
			mediaFile.ReleaseTrack(track);
		}
	}
	return NULL;
}


VideoFileProducer::VideoFileProducer(const char* name):
	TestProducerBase(name)
{
	fFile.SetTo("/boot/home/Video/[Beat Saber] K_DA - DRUM GO DUM - League of Legends (EXPERT+)-mc-dnHVz5Sg.mp4", B_READ_ONLY);
	if (fFile.InitCheck() < B_OK) {abort();}
	fMediaFile.SetTo(new BMediaFile(&fFile));
	media_format mf;
	if (fMediaFile->InitCheck() < B_OK) {abort();}
	fVideoTrack = GetVideoTrack(*fMediaFile.Get(), mf);
	if (fVideoTrack == NULL) {abort();}
	fVideoTrack->DecodedFormat(&mf);
	fRect.Set(0, 0, mf.u.encoded_video.output.display.line_width - 1.0, mf.u.encoded_video.output.display.line_count - 1.0);
	fBitmap.SetTo(new BBitmap(fRect, mf.u.raw_video.display.format));
}

VideoFileProducer::~VideoFileProducer()
{
	fMediaFile->ReleaseTrack(fVideoTrack); fVideoTrack = NULL;
}


void VideoFileProducer::Prepare(BRegion& dirty)
{
	dirty.Include(fRect);
}

void VideoFileProducer::Restore(const BRegion& dirty)
{
	int64 frameCnt = 0;
	media_header mh;
	status_t res = fVideoTrack->ReadFrames(fBitmap->Bits(), &frameCnt, &mh);
	if (frameCnt <= 0 || res < B_OK) {
		int64 time = 0;
		fVideoTrack->SeekToTime(&time);
		res = fVideoTrack->ReadFrames(fBitmap->Bits(), &frameCnt, &mh);
	}
	if (frameCnt <= 0 || res < B_OK) {
		FillRegion(dirty, 0xff000000);
		return;
	}
	RasBuf32 dstRb = RenderBufferRasBuf();
	RasBuf32 srcRb = {
		.colors = (uint32*)fBitmap->Bits(),
		.stride = fBitmap->BytesPerRow() / 4,
		.width = (int32)fBitmap->Bounds().Width() + 1,
		.height = (int32)fBitmap->Bounds().Height() + 1,
	};
	dstRb.Blit(srcRb, B_ORIGIN);
}


void VideoFileProducer::Connected(bool isActive)
{
	if (isActive) {
		int64 time = 0;
		fVideoTrack->SeekToTime(&time);
	}
	TestProducerBase::Connected(isActive);
}

void VideoFileProducer::SwapChainChanged(bool isValid)
{
	if (!isValid) {
		fMessageRunner.Unset();
	}
	TestProducerBase::SwapChainChanged(isValid);
}

void VideoFileProducer::Presented(const PresentedInfo &presentedInfo)
{
	// printf("VideoFileProducer::Presented()\n");
	//fMessageRunner.SetTo(new BMessageRunner(BMessenger(this), BMessage(stepMsg), 1000000/60, 1));
	TestProducerBase::Presented(presentedInfo);
	Produce();
}

void VideoFileProducer::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case stepMsg: {
		Produce();
		return;
	}
	}
	TestProducerBase::MessageReceived(msg);
};
