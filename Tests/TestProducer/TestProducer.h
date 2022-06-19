#ifndef _TESTPRODUCER_H_
#define _TESTPRODUCER_H_

#include "TestProducerBase.h"
#include <MessageRunner.h>


class TestProducer final: public TestProducerBase
{
private:
	enum {
		stepMsg = videoNodeLastMsg + 1,
	};

	ObjectDeleter<BMessageRunner> fMessageRunner;
	bigtime_t fStartTime;
	uint32 fSequence;
	BRect fRect;

	void Layout();

protected:
	void Prepare(BRegion& dirty) final;
	void Restore(const BRegion& dirty) final;

public:
	TestProducer(const char* name);

	void Connected(bool isActive) final;
	void SwapChainChanged(bool isValid) final;
	void Presented(const PresentedInfo &presentedInfo) final;
	void MessageReceived(BMessage* msg) final;
};


#endif	// _TESTPRODUCER_H_
