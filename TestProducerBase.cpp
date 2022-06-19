#include "TestProducerBase.h"

#include <Application.h>


void TestProducerBase::Produce()
{
	if (!SwapChainValid() /*|| RenderBufferId() < 0*/) return;
	
	if (RenderBufferId() < 0) {
		printf("[!] RenderBufferId() < 0\n");
		BRegion dirty;
		Prepare(dirty);
		return;
	}

	switch (GetSwapChain().presentEffect) {
		case presentEffectSwap: {
			BRegion dirty;
			Prepare(dirty);
			BRegion combinedDirty(dirty);
			if (fValidPrevBufCnt < 2) {
				const VideoBuffer& buf = *RenderBuffer();
				combinedDirty.Set(BRect(0, 0, buf.format.width - 1, buf.format.height - 1));
				fValidPrevBufCnt++;
			} else {
				combinedDirty.Include(&fPrevDirty);
			}
			Restore(combinedDirty);
			fPrevDirty = dirty;
			Present(fValidPrevBufCnt == 1 ? &combinedDirty : &dirty);
			break;
		}
		case presentEffectCopy:
		default: {
			BRegion dirty;
			Prepare(dirty);
			if (fValidPrevBufCnt < 1) {
				const VideoBuffer& buf = *RenderBuffer();
				dirty.Set(BRect(0, 0, buf.format.width - 1, buf.format.height - 1));
				fValidPrevBufCnt++;
			}
			Restore(dirty);
			Present(&dirty);
			break;
		}
	}
}


void TestProducerBase::FillRegion(const BRegion& region, uint32 color)
{
	RasBuf32 rb = RenderBufferRasBuf();
	for (int32 i = 0; i < region.CountRects(); i++) {
		clipping_rect rect = region.RectAtInt(i);
		rb.Clip2(rect.left, rect.top, rect.right + 1, rect.bottom + 1).Clear(color);
	}
}


TestProducerBase::TestProducerBase(const char* name):
	VideoProducer(name)
{
}

TestProducerBase::~TestProducerBase()
{
	printf("-TestProducer: "); WriteMessenger(BMessenger(this)); printf("\n");
}

void TestProducerBase::Connected(bool isActive)
{
	if (isActive) {
		printf("TestProducer: connected to ");
		WriteMessenger(Link());
		printf("\n");

		SwapChainSpec spec;
		spec.size = sizeof(SwapChainSpec);
		spec.presentEffect = presentEffectSwap;
		spec.bufferCnt = 2;
		spec.kind = bufferRefArea;
		spec.colorSpace = B_RGBA32;
		if (RequestSwapChain(spec) < B_OK) {
			printf("[!] can't request swap chain\n");
			exit(1);
		}
	} else {
		printf("TestProducer: disconnected\n");
		be_app_messenger.SendMessage(B_QUIT_REQUESTED);
	}
}

void TestProducerBase::SwapChainChanged(bool isValid)
{
	VideoProducer::SwapChainChanged(isValid);
	printf("TestProducer::SwapChainChanged(%d)\n", isValid);

	fSwapChainBind.Unset();

	if (isValid) {
		DumpSwapChain(GetSwapChain());
		fSwapChainBind.ConnectTo(GetSwapChain());
		fValidPrevBufCnt = 0;
		Produce();
	}
}
