#include "ConsumerView.h"
#include "VideoBufferUtils.h"

#include <stdio.h>
#include <Bitmap.h>
#include <Region.h>
#include <Looper.h>
#include <private/shared/AutoLocker.h>

#define CheckRet(err) {status_t _err = (err); if (_err < B_OK) return _err;}


ViewConsumer::ViewConsumer(const char* name, BView* view): VideoConsumer(name), fView(view)
{
}

ViewConsumer::~ViewConsumer()
{
	printf("-ViewConsumer: "); WriteMessenger(BMessenger(this)); printf("\n");
}


void ViewConsumer::Connected(bool isActive)
{
	if (isActive) {
		printf("ViewConsumer: connected to ");
		WriteMessenger(Link());
		printf("\n");
	} else {
		printf("ViewConsumer: disconnected\n");
		SetSwapChain(NULL);
		fBitmaps.Unset();
		fView->Invalidate();
	}
}

status_t ViewConsumer::SetupSwapChain()
{
	uint32 bufferCnt = 16 /* spec.bufferCnt */;
	
	fBitmaps.SetTo(new ObjectDeleter<BBitmap>[bufferCnt]);
	for (uint32 i = 0; i < bufferCnt; i++) {
		fBitmaps[i].SetTo(new BBitmap(fView->Bounds(), B_BITMAP_CLEAR_TO_WHITE, B_RGBA32 /* spec.bufferSpecs[i].colorSpace */));
	}
	ArrayDeleter<VideoBuffer> buffers(new VideoBuffer[bufferCnt]);
	SwapChain swapChain {
		.size = sizeof(SwapChain),
		.presentEffect = presentEffectSwap,
		.bufferCnt = bufferCnt,
		.buffers = buffers.Get()
	};
	for (uint32 i = 0; i < bufferCnt; i++) {
		buffers[i].id = i;
		CheckRet(VideoBufferFromBitmap(buffers[i], *fBitmaps[i].Get()));
	}
	SetSwapChain(&swapChain);
	return B_OK;
}

status_t ViewConsumer::SwapChainRequested(const SwapChainSpec& spec)
{
		printf("ViewConsumer::SwapChainRequested(%" B_PRIuSIZE ")\n", spec.bufferCnt);

		return SetupSwapChain();
}

void ViewConsumer::Present(int32 bufferId, const BRegion* dirty)
{
	//printf("ViewConsumer::Present()\n");
	AutoLocker<BHandler, AutoLockerHandlerLocking<BHandler>> lock(fView);
	if (GetSwapChain().presentEffect == presentEffectCopy) {
		BBitmap *dstBmp = fBitmaps[DisplayBufferId()].Get();
		BBitmap *srcBmp = fBitmaps[bufferId].Get();
		if (dirty != NULL) {
			for (int32 i = 0; i < dirty->CountRects(); i++) {
				BRect rect = dirty->RectAt(i);
				//printf("  Rect(%g, %g, %g, %g)\n", rect.left, rect.top, rect.right, rect.bottom);
				dstBmp->ImportBits(srcBmp, rect.LeftTop(), rect.LeftTop(), rect.Size());
			}
		} else {
			dstBmp->ImportBits(srcBmp);
		}
	}
	if (dirty != NULL) {
		fView->Invalidate(dirty);
	} else {
		fView->Invalidate();
	}
	//printf("[WAIT]"); fgetc(stdin);
	Presented();
}


void ConsumerView::AttachedToWindow()
{
	fConsumer.SetTo(new ViewConsumer("testConsumer", this));
	Looper()->AddHandler(fConsumer.Get());
	printf("+ViewConsumer: "); WriteMessenger(BMessenger(fConsumer.Get())); printf("\n");
}

void ConsumerView::DetachedFromWindow()
{
	fConsumer.Unset();
}

void ConsumerView::FrameResized(float newWidth, float newHeight)
{
	if (fConsumer->SwapChainValid() && fConsumer->OwnsSwapChain())
		fConsumer->SetupSwapChain();
	Invalidate();
}

void ConsumerView::Draw(BRect dirty)
{
	BBitmap* bmp = fConsumer->DisplayBitmap();
	if (bmp != NULL) {
		DrawBitmap(bmp);
		return;
	}
}
