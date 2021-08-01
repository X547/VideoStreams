#include "VideoConsumer.h"

#include <Region.h>


#define CheckRet(err) {status_t _err = (err); if (_err < B_OK) return _err;}


VideoConsumer::VideoConsumer(const char* name):
	VideoNode(name), fDisplayBufferId(-1), fOldDisplayBufferId(-1)
{}

VideoConsumer::~VideoConsumer()
{}


void VideoConsumer::SwapChainChanged(bool isValid)
{
	fDisplayBufferId = -1;
	fOldDisplayBufferId = -1;
}


VideoBuffer* VideoConsumer::DisplayBuffer()
{
	if (!SwapChainValid() || fDisplayBufferId < 0)
		return NULL;

	return &GetSwapChain().buffers[fDisplayBufferId];
}


status_t VideoConsumer::Presented()
{
	if (!IsConnected() || !SwapChainValid())
		return B_NOT_ALLOWED;

	BMessage msg(videoNodePresentedMsg);
	if (fOldDisplayBufferId >= 0)
		msg.AddInt32("recycleId", fOldDisplayBufferId);
	fOldDisplayBufferId = fDisplayBufferId;
	CheckRet(Link().SendMessage(&msg));
	
	return B_OK;
}

void VideoConsumer::Present(const BRegion* dirty)
{}


void VideoConsumer::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case videoNodePresentMsg: {
		// TODO: don't assume 2 buffers and specific buffer IDs
		int32 bufferId;
		if (msg->FindInt32("bufferId", &bufferId) < B_OK)
			return;
		fDisplayBufferId = bufferId;
		Present(NULL);
		return;
	}
	}
	VideoNode::MessageReceived(msg);
}
