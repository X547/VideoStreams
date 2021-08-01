#ifndef _VIDEOCONSUMER_H_
#define _VIDEOCONSUMER_H_

#include <VideoNode.h>

class BRegion;

class _EXPORT VideoConsumer: public VideoNode
{
private:
	int32 fDisplayBufferId;
	int32 fOldDisplayBufferId;

public:
	VideoConsumer(const char* name = NULL);
	virtual ~VideoConsumer();

	virtual void SwapChainChanged(bool isValid);

	uint32 DisplayBufferId() {return fDisplayBufferId;}
	VideoBuffer* DisplayBuffer();

	status_t Presented();
	virtual void Present(const BRegion* dirty);

	virtual void MessageReceived(BMessage* msg);
};

#endif	// _VIDEOCONSUMER_H_
