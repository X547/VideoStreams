#ifndef _TESTPRODUCER_H_
#define _TESTPRODUCER_H_

#include <type_traits>

#include <VideoProducer.h>
#include <VideoBuffer.h>
#include <MessageRunner.h>
#include <AutoDeleter.h>

#include <amdgpu.h>


typedef CObjectDeleter<amdgpu_device, int, amdgpu_device_deinitialize> AmdgpuDeviceRef;
typedef CObjectDeleter<amdgpu_bo, int, amdgpu_bo_free> AmdgpuBoRef;

class TestProducer final: public VideoProducer {
private:
	AmdgpuDeviceRef fDev;
	uint32 fBufferCnt{};
	ArrayDeleter<AmdgpuBoRef> fBufs;

	void Produce();

public:
	TestProducer(const char* name);
	virtual ~TestProducer();

	void Connected(bool isActive) final;
	void SwapChainChanged(bool isValid) final;
	void Presented(const PresentedInfo &presentedInfo) final;
	void MessageReceived(BMessage* msg) final;
};


#endif	// _TESTPRODUCER_H_
