#ifndef _TESTPRODUCER_H_
#define _TESTPRODUCER_H_

#include <VideoStreams/VideoProducer.h>
#include <MessageRunner.h>
#include <AutoDeleter.h>

#include <amdgpu.h>


class TestProducer final: public VideoProducer
{
private:
	enum {
		stepMsg = videoNodeLastMsg + 1,
	};

	amdgpu_device_handle fDev{};
	uint32 fBufferCnt{};
	ArrayDeleter<amdgpu_bo_handle> fBufs;

	void Produce();

public:
	TestProducer(const char* name);
	virtual ~TestProducer();

	void Connected(bool isActive) final;
	void SwapChainChanged(bool isValid) final;
	void Presented() final;
	void MessageReceived(BMessage* msg) final;
};


#endif	// _TESTPRODUCER_H_
