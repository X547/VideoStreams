#include "ConsumerView.h"
#include "VideoBufferUtils.h"
#include <VideoBufferBindSW.h>

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
	SwapChainBindSW fSwapChainBind;
	uint32 fBitmapCnt = 0;
	ArrayDeleter<ObjectDeleter<BBitmap> > fBitmaps;
	BBitmap *fDisplayBitmap = NULL;
	ObjectDeleter<BBitmap> fDisplayBitmapDeleter;

	status_t SetupSwapChain();

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
		fBitmaps.Unset();
		fView->Invalidate();
	}
}

status_t ViewConsumer::SetupSwapChain()
{
	uint32 bufferCnt = 2;

	// keep old display bitmap
	if (fDisplayBitmap != NULL) {
		for (uint32 i = 0; i < fBitmapCnt; i++) {
			if (fBitmaps[i].Get() == fDisplayBitmap) {
				fDisplayBitmapDeleter.SetTo(fBitmaps[i].Detach());
			}
		}
	}

	fBitmapCnt = bufferCnt;
	fBitmaps.SetTo(new ObjectDeleter<BBitmap>[fBitmapCnt]);
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

void ViewConsumer::SwapChainChanged(bool isValid)
{
	VideoConsumer::SwapChainChanged(isValid);
	if (!isValid) {
		fSwapChainBind.Unset();
		fBitmaps.Unset();
		return;
	}
	if (!OwnsSwapChain()) {
		DumpSwapChain(GetSwapChain());
		fSwapChainBind.ConnectTo(GetSwapChain());
		fBitmaps.SetTo(new ObjectDeleter<BBitmap>[GetSwapChain().bufferCnt]);
		for (uint32 i = 0; i < GetSwapChain().bufferCnt; i++) {
			const auto &buffer = GetSwapChain().buffers[i];
			const auto &mappedBuffer = fSwapChainBind.Buffers()[i];
			printf("mappedBuffers[%" B_PRIu32 "]: %" B_PRId32 "\n", i, mappedBuffer.area->GetArea());
			fBitmaps[i].SetTo(new BBitmap(
				mappedBuffer.area->GetArea(),
				buffer.ref.offset,
				BRect(0, 0, buffer.format.width - 1, buffer.format.height - 1),
				0,
				buffer.format.colorSpace,
				buffer.format.bytesPerRow
			));
		}
	}
}


void ViewConsumer::Present(int32 bufferId, const BRegion* dirty)
{
	//printf("ViewConsumer::Present()\n");
	fDisplayBitmapDeleter.Unset();
	fDisplayBitmap = fBitmaps[bufferId].Get();

	switch (GetSwapChain().presentEffect) {
		case presentEffectCopy: {
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
	//printf("[WAIT]"); fgetc(stdin);
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
	BBitmap* bmp = fConsumer->fDisplayBitmap;
	if (bmp == NULL) return;
	DrawBitmap(bmp);
}


VideoConsumer* ConsumerView::GetConsumer()
{
	return fConsumer.Get();
}
