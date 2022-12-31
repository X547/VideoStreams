#include "TestProducer.h"


TestProducer::TestProducer(const char* name):
	TestProducerBase(name),
	fSequence(0)
{
}


void TestProducer::Layout()
{
	enum {
		minX = 32,
		minY = 32,
		period = 12
	};

	static const float stepDelay = 1/60.0;

	const VideoBuffer& buf = *RenderBuffer();
	int32 maxX = buf.format.width - 32;
	int32 maxY = buf.format.height - 32;

	float fTime;
	BPoint fPos;
	
	//fTime = fSequence*stepDelay;
	fTime = float(system_time() - fStartTime)/1000000.0f;
	float a = 2*M_PI*fTime/period;
	fPos.x = minX + (maxX - minX)*(cosf(a) + 1)/2;
	fPos.y = minY + (maxY - minY)*(sinf(2*a) + 1)/2;

	fRect.Set(fPos.x - 16, fPos.y - 16, fPos.x + 16 - 1, fPos.y + 16 - 1);
}


void TestProducer::Prepare(BRegion& dirty)
{
	dirty.Include(fRect);
	Layout();
	dirty.Include(fRect);
}

void TestProducer::Restore(const BRegion& dirty)
{
	FillRegion(dirty, 0x80ffffff);
	BRegion region = BRegion(fRect);
	region.IntersectWith(&dirty);
	FillRegion(region, 0x80000000);
	fSequence++;
}


void TestProducer::Connected(bool isActive)
{
	if (isActive) {
		fSequence = 0;
		fStartTime = system_time();
	}
	TestProducerBase::Connected(isActive);
}

void TestProducer::SwapChainChanged(bool isValid)
{
	if (!isValid) {
		fMessageRunner.Unset();
	}
	TestProducerBase::SwapChainChanged(isValid);
}

void TestProducer::Presented(const PresentedInfo &presentedInfo)
{
	TestProducerBase::Presented(presentedInfo);
	// printf("TestProducer::Presented()\n");
	Produce();
	//fMessageRunner.SetTo(new BMessageRunner(BMessenger(this), BMessage(stepMsg), 1000000/60, 1));
}

void TestProducer::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case stepMsg: {
		Produce();
		return;
	}
	}
	TestProducerBase::MessageReceived(msg);
};
