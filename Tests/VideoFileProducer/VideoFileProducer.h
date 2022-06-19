#ifndef _TESTPRODUCER_H_
#define _TESTPRODUCER_H_

#include "TestProducerBase.h"
#include <MessageRunner.h>

#include <File.h>

class BMediaFile;
class BMediaTrack;


class VideoFileProducer final: public TestProducerBase
{
private:
	enum {
		stepMsg = videoNodeLastMsg + 1,
	};

	ObjectDeleter<BMessageRunner> fMessageRunner;
	ObjectDeleter<BBitmap> fBitmap;
	BRect fRect;

	BFile fFile;
	ObjectDeleter<BMediaFile> fMediaFile;
	BMediaTrack* fVideoTrack;

protected:
	void Prepare(BRegion& dirty) final;
	void Restore(const BRegion& dirty) final;

public:
	VideoFileProducer(const char* name);
	virtual ~VideoFileProducer();

	void Connected(bool isActive) final;
	void SwapChainChanged(bool isValid) final;
	void Presented(const PresentedInfo &presentedInfo) final;
	void MessageReceived(BMessage* msg) final;
};


#endif	// _TESTPRODUCER_H_
