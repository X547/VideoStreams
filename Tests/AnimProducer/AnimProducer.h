#ifndef _ANIMPRODUCER_H_
#define _ANIMPRODUCER_H_

#include "TestProducerBase.h"
#include <MessageRunner.h>
#include <ObjectList.h>
#include <Bitmap.h>


class AnimProducer final: public TestProducerBase
{
private:
	enum {
		stepMsg = videoNodeLastMsg + 1,
	};

	BMessenger fCompositor, fSurface;
	ObjectDeleter<BMessageRunner> fMessageRunner;
	uint32 fCurFrame;
	bool fFlag;

	BObjectList<BBitmap> fBitmaps;
	
	void Load(const char *path);

protected:
	void Prepare(BRegion& dirty) final;
	void Restore(const BRegion& dirty) final;

public:
	AnimProducer(const char* name);
	
	void SetSurface(BMessenger compositor = BMessenger(), BMessenger surface = BMessenger());

	void Connected(bool isActive) final;
	void SwapChainChanged(bool isValid) final;
	void Presented(const PresentedInfo &presentedInfo) final;
	void MessageReceived(BMessage* msg) final;
	
	inline BBitmap* CurBitmap();
	inline BRect Bounds();
};


BBitmap* AnimProducer::CurBitmap()
{
	return fBitmaps.ItemAt(fCurFrame);
}

BRect AnimProducer::Bounds()
{
	return CurBitmap()->Bounds();
}


#endif	// _ANIMPRODUCER_H_
