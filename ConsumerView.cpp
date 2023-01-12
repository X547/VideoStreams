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

	void KeepOldDisplayBitmap();

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

void ViewConsumer::KeepOldDisplayBitmap()
{
	return;
	printf("KeepOldDisplayBitmap(), fDisplayBitmap: %p\n", fDisplayBitmap);
	if (fDisplayBitmap == NULL) {
		return;
	}
	for (uint32 i = 0; i < fBitmapCnt; i++) {
			printf("  bitmap %p\n", fSwapChainBind.Buffers()[i].bitmap.Get());
		if (fSwapChainBind.Buffers()[i].bitmap.Get() == fDisplayBitmap) {
			printf("  bitmap %p kept\n", fDisplayBitmap);
			fDisplayBitmapDeleter.SetTo(fSwapChainBind.Buffers()[i].bitmap.Detach());
			return;
		}
	}
}

status_t ViewConsumer::SwapChainRequested(const SwapChainSpec& spec)
{
	printf("ViewConsumer::SwapChainRequested(%" B_PRIuSIZE ")\n", spec.bufferCnt);

	KeepOldDisplayBitmap();

	fBitmapCnt = spec.bufferCnt;
	ObjectDeleter<SwapChain> swapChain;
	CheckRet(fSwapChainBind.Alloc(swapChain, spec));
	SetSwapChain(swapChain.Get());

	return B_OK;
}

void ViewConsumer::SwapChainChanged(bool isValid)
{
	printf("ViewConsumer::SwapChainChanged(%d)\n", isValid);
	VideoConsumer::SwapChainChanged(isValid);
	if (!isValid) {
		KeepOldDisplayBitmap();
		fSwapChainBind.Unset();
		fBitmapCnt = 0;
		return;
	}
	if (!OwnsSwapChain()) {
		DumpSwapChain(GetSwapChain());
		KeepOldDisplayBitmap();
		fSwapChainBind.ConnectTo(GetSwapChain());
		fBitmapCnt = GetSwapChain().bufferCnt;
	}
}


void ViewConsumer::Present(int32 bufferId, const BRegion* dirty)
{
	//fDisplayBitmapDeleter.Unset();
	//fDisplayBitmap = fSwapChainBind.Buffers()[bufferId].bitmap.Get();
	fView->SetViewBitmap(fSwapChainBind.Buffers()[bufferId].bitmap.Get(), B_FOLLOW_TOP | B_FOLLOW_LEFT, 0);
	printf("ViewConsumer::Present(%" B_PRId32 "), displayBitmap: %p\n", bufferId, fDisplayBitmap);
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
	return;
	printf("ConsumerView::Draw(), displayBitmap: %p, displayBitmap->Area(): %" B_PRId32 "\n", fConsumer->fDisplayBitmap, fConsumer->fDisplayBitmap == NULL ? -1 : fConsumer->fDisplayBitmap->Area());
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
