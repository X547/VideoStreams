#include "VideoProducer.h"
#include "VideoBuffer.h"

#include <Region.h>
#include <stdio.h>


#define CheckRet(err) {status_t _err = (err); if (_err < B_OK) return _err;}


VideoProducer::VideoProducer(const char* name):
	VideoNode(name), fEra(0)
{}

VideoProducer::~VideoProducer()
{}


void VideoProducer::SwapChainChanged(bool isValid)
{
	int32 bufferCnt = isValid ? GetSwapChain().bufferCnt : 0;
	fBufferPool.SetMaxLen(bufferCnt);
	// buffer 0 is fixed present buffer
	for (int32 i = (!isValid || GetSwapChain().presentEffect == presentEffectSwap) ? 0 : 1; i < bufferCnt; i++) {
		fBufferPool.Add(i);
	}
}


int32 VideoProducer::RenderBufferId()
{
	return fBufferPool.Begin();
}

int32 VideoProducer::AllocBuffer()
{
	return fBufferPool.Remove();
}

bool VideoProducer::FreeBuffer(int32 bufferId)
{
	return fBufferPool.Add(bufferId);
}

VideoBuffer* VideoProducer::RenderBuffer()
{
	int32 bufferId = RenderBufferId();
	if (bufferId < 0)
		return NULL;
	return &GetSwapChain().buffers[bufferId];
}


status_t VideoProducer::Present(int32 bufferId, const BRegion* dirty)
{
	if (!IsConnected() || !SwapChainValid())
		return B_NOT_ALLOWED;

	BMessage msg(videoNodePresentMsg);
	msg.AddInt32("bufferId", bufferId);
	msg.AddUInt32("era", fEra);

	if (dirty != NULL) {
		for (int32 i = 0; i < dirty->CountRects(); i++) {
			msg.AddRect("dirty", dirty->RectAt(i));
		}
	}

	CheckRet(Link().SendMessage(&msg));

	fEra++;

	return B_OK;
}

status_t VideoProducer::Present(const BRegion* dirty)
{
	if ((RenderBufferId() < 0))
		return B_NOT_ALLOWED;

	CheckRet(Present(RenderBufferId(), dirty));
	AllocBuffer();
	return B_OK;
}

status_t VideoProducer::PresentedInt(int32 recycleId, const PresentedInfo &presentedInfo)
{
	if (recycleId >= 0) {
		fBufferPool.Add(recycleId);
	}
	Presented(presentedInfo);
	return B_OK;
}

void VideoProducer::Presented(const PresentedInfo &presentedInfo)
{}

status_t VideoProducer::GetConsumerInfo(ConsumerInfo &info)
{
	return ENOSYS;
}

void VideoProducer::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case videoNodePresentedMsg: {
		PresentedInfo presentedInfo{};
		int32 recycleId;
		if (msg->FindInt32("recycleId", &recycleId) < B_OK) recycleId = -1;
		msg->FindUInt32("era", &presentedInfo.era);
		if (msg->FindBool("suboptimal", &presentedInfo.suboptimal) < B_OK) presentedInfo.suboptimal = false;
		if (presentedInfo.suboptimal) {
			if (msg->FindInt32("width", &presentedInfo.width) < B_OK) presentedInfo.width = 0;
			if (msg->FindInt32("height", &presentedInfo.height) < B_OK) presentedInfo.height = 0;
		}
		PresentedInt(recycleId, presentedInfo);
		return;
	}
	}
	VideoNode::MessageReceived(msg);
}
