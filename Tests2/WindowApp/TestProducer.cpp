#include "TestProducer.h"


static void FillRegion(const RasBuf32 &rb, const BRegion& region, uint32 color)
{
	for (int32 i = 0; i < region.CountRects(); i++) {
		clipping_rect rect = region.RectAtInt(i);
		rb.Clip2(rect.left, rect.top, rect.right + 1, rect.bottom + 1).Clear(color);
	}
}


TestProducer::TestProducer(const char* name):
	VideoProducer(name),
	fSequence(0)
{
}


//#pragma mark - test animation generator

void TestProducer::Layout(int32 bufferId)
{
	static float minX = 32;
	static float minY = 32;
	static float period = 12;

	const VideoBuffer& buf = GetSwapChain().buffers[bufferId];
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


void TestProducer::Prepare(int32 bufferId, BRegion& dirty)
{
	dirty.Include(fRect);
	Layout(bufferId);
	dirty.Include(fRect);
}

void TestProducer::Restore(int32 bufferId, const BRegion& dirty)
{
	RasBuf32 rb = GetRasBuf(bufferId);
	FillRegion(rb, dirty, 0x80cccccc);
	BRegion region = BRegion(fRect);
	region.IntersectWith(&dirty);
	FillRegion(rb, region, 0x80000000);
	fSequence++;
}


//#pragma mark -

void TestProducer::UpdateSwapChain(int32 width, int32 height)
{
	bool ownsSwapChain = false;
	SwapChainSpec spec {
		.size = sizeof(SwapChainSpec),
		.presentEffect = presentEffectSwap,
		.bufferCnt = 2,
		.kind = bufferRefArea,
		.width = width,
		.height = height,
		.colorSpace = B_RGBA32
	};
	if (!ownsSwapChain) {
		if (RequestSwapChain(spec) < B_OK) {
			printf("[!] can't request swap chain\n");
			abort();
		}
	} else {
		ObjectDeleter<SwapChain> swapChain;
		fSwapChainBind.Alloc(swapChain, spec);
		DumpSwapChain(*swapChain.Get());
		SetSwapChain(swapChain.Get());
	}
}


void TestProducer::Connected(bool isActive)
{
	if (isActive) {
		fSequence = 0;
		fStartTime = system_time();
		UpdateSwapChain();
	}
}

void TestProducer::SwapChainChanged(bool isValid)
{
	VideoProducer::SwapChainChanged(isValid);
	printf("TestProducer::SwapChainChanged(%d)\n", isValid);
	if (!isValid) {
		fMessageRunner.Unset();
		if (!OwnsSwapChain()) {
			fSwapChainBind.Unset();
		}
		return;
	}

	//DumpSwapChain(GetSwapChain());
	if (!OwnsSwapChain()) {
		fSwapChainBind.ConnectTo(GetSwapChain());
	}
	fValidPrevBufCnt = 0;
	Produce();
}

void TestProducer::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case stepMsg: {
		Produce();
		return;
	}
	}
	VideoProducer::MessageReceived(msg);
};


void TestProducer::Produce()
{
	if (!SwapChainValid()) return;

	int32 bufferId = AllocBuffer();

	if (bufferId < 0) {
		printf("[!] AllocBuffer failed\n");
		return;
	}

	switch (GetSwapChain().presentEffect) {
		case presentEffectSwap: {
			BRegion dirty;
			Prepare(bufferId, dirty);
			BRegion combinedDirty(dirty);
			if (fValidPrevBufCnt < 2) {
				const VideoBuffer& buf = GetSwapChain().buffers[bufferId];
				combinedDirty.Set(BRect(0, 0, buf.format.width - 1, buf.format.height - 1));
				fValidPrevBufCnt++;
			} else {
				combinedDirty.Include(&fPrevDirty);
			}
			Restore(bufferId, combinedDirty);
			fPrevDirty = dirty;
			fPending++;
			Present(bufferId, fValidPrevBufCnt == 1 ? &combinedDirty : &dirty);
			break;
		}
		case presentEffectCopy:
		default: {
			BRegion dirty;
			Prepare(bufferId, dirty);
			if (fValidPrevBufCnt < 1) {
				const VideoBuffer& buf = GetSwapChain().buffers[bufferId];
				dirty.Set(BRect(0, 0, buf.format.width - 1, buf.format.height - 1));
				fValidPrevBufCnt++;
			}
			Restore(bufferId, dirty);
			fPending++;
			Present(bufferId, &dirty);
			break;
		}
	}
}

void TestProducer::Presented(const PresentedInfo &presentedInfo)
{
	fPending--;
	if (fPending != 0)
		printf("Presented(), pending: %" B_PRId32 "\n", fPending);
	if (presentedInfo.suboptimal) {
		if (fPending == 0) {
			UpdateSwapChain(presentedInfo.width, presentedInfo.height);
		}
	} else {
		if (fPending == 0) {
			Produce();
		}
	}
	//fMessageRunner.SetTo(new BMessageRunner(BMessenger(this), BMessage(stepMsg), 1000000/60, 1));
	VideoProducer::Presented(presentedInfo);
}
