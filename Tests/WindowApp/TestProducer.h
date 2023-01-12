#ifndef _TESTPRODUCER_H_
#define _TESTPRODUCER_H_

#include "TestProducerBase.h"
#include <MessageRunner.h>


class TestProducer final: public VideoProducer
{
private:
	enum {
		stepMsg = videoNodeLastMsg + 1,
	};

	SwapChainBindSW fSwapChainBind;
	uint32 fValidPrevBufCnt;
	BRegion fPrevDirty;

	ObjectDeleter<BMessageRunner> fMessageRunner;
	bigtime_t fStartTime;
	uint32 fSequence;
	BRect fRect;
	int32 fPending = 0;

	void Layout(int32 bufferId);
	void UpdateSwapChain(int32 width = 0, int32 height = 0);

protected:
	inline RasBuf32 GetRasBuf(int32 bufferId);
	void Produce();

	void Prepare(int32 bufferId, BRegion& dirty);
	void Restore(int32 bufferId, const BRegion& dirty);

public:
	TestProducer(const char* name);

	void Connected(bool isActive) final;
	void SwapChainChanged(bool isValid) final;
	void Presented(const PresentedInfo &presentedInfo) final;
	void MessageReceived(BMessage* msg) final;
};


RasBuf32 TestProducer::GetRasBuf(int32 bufferId)
{
	if (bufferId < 0) {
		RasBuf32 rb {};
		return rb;
	}
	const VideoBuffer &buf = GetSwapChain().buffers[bufferId];
	const auto &mappedBuffer = fSwapChainBind.Buffers()[bufferId];
	RasBuf32 rb = {
		.colors = (uint32*)mappedBuffer.bits,
		.stride = buf.format.bytesPerRow / 4,
		.width  = buf.format.width,
		.height = buf.format.height,		
	};
	return rb;
}


#endif	// _TESTPRODUCER_H_
