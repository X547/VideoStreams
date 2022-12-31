#include "AnimProducer.h"

#include <CompositeConsumer.h>

#include <Bitmap.h>
#include <File.h>
#include <TranslationUtils.h>


AnimProducer::AnimProducer(const char* name):
	TestProducerBase(name),
	fCurFrame(0)
{
	Load("/Haiku 64 (USB)/home/Tests/VideoStreams/rsrc/comipo24bit");
}


void AnimProducer::Prepare(BRegion& dirty)
{
	dirty.Set(CurBitmap()->Bounds());
}

void AnimProducer::Restore(const BRegion& dirty)
{
	RasBuf32 rb = RenderBufferRasBuf();
	BBitmap *bmp = CurBitmap();
	RasBuf32 srcRb = {
		.colors = (uint32*)bmp->Bits(),
		.stride = bmp->BytesPerRow() / 4,
		.width = (int32)bmp->Bounds().Width() + 1,
		.height = (int32)bmp->Bounds().Height() + 1,		
	};
	rb.Blit(srcRb, BPoint(0, 0));
	fCurFrame = (fCurFrame + 1)%fBitmaps.CountItems();
	
	CompositeConsumer* handler = dynamic_cast<CompositeConsumer*>(Link().Target(NULL));
	if (handler != NULL) {
		SurfaceUpdate update = {
			.valid = (1 << surfaceFrame)
		};
		handler->Base()->GetSurface(handler, update);
		update.frame.OffsetBySelf(-2, 0);
		if (update.frame.left < 0) {
			update.frame.OffsetBySelf(512, 0);
		}
		handler->Base()->UpdateSurface(handler, update);
	}
}


void AnimProducer::Connected(bool isActive)
{
	if (isActive) {
		fCurFrame = 0;
	}
	TestProducerBase::Connected(isActive);
}

void AnimProducer::SwapChainChanged(bool isValid)
{
	if (!isValid) {
		fMessageRunner.Unset();
	}
	TestProducerBase::SwapChainChanged(isValid);
}

void AnimProducer::Presented(const PresentedInfo &presentedInfo)
{
	TestProducerBase::Presented(presentedInfo);
	// printf("AnimProducer::Presented()\n");
	fMessageRunner.SetTo(new BMessageRunner(BMessenger(this), BMessage(stepMsg), 1000000/60, 1));
}

void AnimProducer::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case stepMsg: {
		Produce();
		return;
	}
	}
	TestProducerBase::MessageReceived(msg);
};


void AnimProducer::Load(const char *path)
{
	fBitmaps.MakeEmpty();
	for (int i = 0;; i++) {
		BString filePath;
		filePath.SetToFormat("%s/comipo24bit%02d.png", path, i + 1);
		BFile file(filePath, B_READ_ONLY);
		if (file.InitCheck() < B_OK) break;
		printf("%s\n", filePath.String());
		fBitmaps.AddItem(BTranslationUtils::GetBitmap(&file));
	}
	printf("%" B_PRId32 " items loaded\n", fBitmaps.CountItems());
}
