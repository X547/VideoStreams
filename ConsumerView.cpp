#include "ConsumerView.h"
#include "VideoBufferUtils.h"
#include <VideoBufferBindBitmap.h>

#include <stdio.h>
#include <Bitmap.h>
#include <Region.h>
#include <Looper.h>
#include <private/shared/AutoLocker.h>

#define CheckRet(err) {status_t _err = (err); if (_err < B_OK) return _err;}


class ViewConsumer final: public VideoConsumer
{
private:
	friend class ConsumerView;

	BView* fView;
	SwapChainBindBitmap fSwapChainBind;
	uint32 fBitmapCnt = 0;
	BBitmap *fDisplayBitmap = NULL;
	ObjectDeleter<BBitmap> fDisplayBitmapDeleter;

public:
	ViewConsumer(const char* name, BView* view);
	virtual ~ViewConsumer();
	
	void Connected(bool isActive) final;
	status_t SwapChainRequested(const SwapChainSpec& spec) final;
	void SwapChainChanged(bool isValid) final;
	void Present(int32 bufferId, const BRegion* dirty) final;
};


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
		fSwapChainBind.Unset();
		fDisplayBitmap = NULL;
		fView->Invalidate();
	}
}

status_t ViewConsumer::SwapChainRequested(const SwapChainSpec& spec)
{
	printf("ViewConsumer::SwapChainRequested(%" B_PRIuSIZE ")\n", spec.bufferCnt);

	// keep old display bitmap
	if (fDisplayBitmap != NULL) {
		for (uint32 i = 0; i < fBitmapCnt; i++) {
			if (fSwapChainBind.Buffers()[i].bitmap.Get() == fDisplayBitmap) {
				fDisplayBitmapDeleter.SetTo(fSwapChainBind.Buffers()[i].bitmap.Detach());
			}
		}
	}

	fBitmapCnt = spec.bufferCnt;
	ObjectDeleter<SwapChain> swapChain;
	fSwapChainBind.Alloc(swapChain, spec);
	SetSwapChain(swapChain.Get());

	return B_OK;
}

void ViewConsumer::SwapChainChanged(bool isValid)
{
	VideoConsumer::SwapChainChanged(isValid);
	if (!isValid) {
		fSwapChainBind.Unset();
		return;
	}
	if (!OwnsSwapChain()) {
		DumpSwapChain(GetSwapChain());
		fSwapChainBind.ConnectTo(GetSwapChain());
	}
}


void ViewConsumer::Present(int32 bufferId, const BRegion* dirty)
{
	fDisplayBitmapDeleter.Unset();
	fDisplayBitmap = fSwapChainBind.Buffers()[bufferId].bitmap.Get();

	switch (GetSwapChain().presentEffect) {
		case presentEffectCopy: {
			BBitmap *dstBmp = fSwapChainBind.Buffers()[DisplayBufferId()].bitmap.Get();
			BBitmap *srcBmp = fSwapChainBind.Buffers()[bufferId].bitmap.Get();
			if (dirty != NULL) {
				for (int32 i = 0; i < dirty->CountRects(); i++) {
					BRect rect = dirty->RectAt(i);
					dstBmp->ImportBits(srcBmp, rect.LeftTop(), rect.LeftTop(), rect.Size());
				}
			} else {
				dstBmp->ImportBits(srcBmp);
			}
			break;
		}
		case presentEffectSwap: {
			break;
		}
	}

	if (dirty != NULL) {
		fView->Invalidate(dirty);
	} else {
		fView->Invalidate();
	}
	VideoBuffer *buffer = &GetSwapChain().buffers[bufferId];
	PresentedInfo presentedInfo {
		.suboptimal = !(
			(buffer->format.width == (int32)fView->Frame().Width() + 1) &&
			(buffer->format.height == (int32)fView->Frame().Height() + 1)
		),
		.width = (int32)fView->Frame().Width() + 1,
		.height = (int32)fView->Frame().Height() + 1,
	};
	Presented(presentedInfo);
}


void ConsumerView::AttachedToWindow()
{
	SetViewColor(B_TRANSPARENT_COLOR);
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
}

void ConsumerView::Draw(BRect dirty)
{
	BRegion region(dirty);
	BBitmap* bmp = fConsumer->fDisplayBitmap;
	if (bmp != NULL) {
		DrawBitmap(bmp, B_ORIGIN);
		region.Exclude(bmp->Bounds());
	}
	FillRegion(&region, B_SOLID_LOW);
}


VideoConsumer* ConsumerView::GetConsumer()
{
	return fConsumer.Get();
}
