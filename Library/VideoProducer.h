#ifndef _VIDEOPRODUCER_H_
#define _VIDEOPRODUCER_H_

#include <VideoNode.h>

class BRegion;


class _EXPORT VideoProducer: public VideoNode
{
private:
	int32 fRenderBufferId;

public:
	VideoProducer(const char* name = NULL);
	virtual ~VideoProducer();

	virtual void SwapChainChanged(bool isValid);

	int32 RenderBufferId() {return fRenderBufferId;}
	VideoBuffer* RenderBuffer();
	status_t Present(BRegion* dirty = NULL);
	virtual void Presented();

	virtual void MessageReceived(BMessage* msg);
};

#endif	// _VIDEOPRODUCER_H_
