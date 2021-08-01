#include "VideoProducer.h"

#include <Region.h>


#define CheckRet(err) {status_t _err = (err); if (_err < B_OK) return _err;}


VideoProducer::VideoProducer(const char* name):
	VideoNode(name), fRenderBufferId(-1)
{}

VideoProducer::~VideoProducer()
{}


void VideoProducer::SwapChainChanged(bool isValid)
{
	if (isValid) {
		fRenderBufferId = 0;
	} else {
		fRenderBufferId = -1;
	}
}


VideoBuffer* VideoProducer::RenderBuffer()
{
	if (!SwapChainValid() || fRenderBufferId < 0)
		return NULL;

	return &GetSwapChain().buffers[fRenderBufferId];
}


status_t VideoProducer::Present(BRegion* dirty)
{
	if (!IsConnected() || !SwapChainValid())
		return B_NOT_ALLOWED;

	BMessage msg(videoNodePresentMsg);
	msg.AddInt32("bufferId", fRenderBufferId);
/*
	if (dirty != NULL)
		msg.AddRegion("dirty", dirty);
*/
	CheckRet(Link().SendMessage(&msg));
	
	return B_OK;
}

void VideoProducer::Presented()
{}


void VideoProducer::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case videoNodePresentedMsg: {
		// TODO: don't assume 2 buffers and specific buffer IDs
		int32 recycleId;
		if (msg->FindInt32("recycleId", &recycleId) < B_OK)
			fRenderBufferId = 1;
		else
			fRenderBufferId = recycleId;
		Presented();
		return;
	}
	}
	VideoNode::MessageReceived(msg);
}
