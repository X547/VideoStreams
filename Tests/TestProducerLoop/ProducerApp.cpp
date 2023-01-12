#include <Application.h>
#include <Messenger.h>
#include <stdio.h>
#include <string.h>
#include <private/shared/AutoDeleter.h>
#include <private/shared/AutoDeleterOS.h>
#include <Region.h>

#include <VideoProducer.h>
#include <VideoBufferBindSW.h>
#include "RasBuf.h"
#include "AppKitPtrs.h"
#include <FindConsumer.h>


static void FillRegion(const RasBuf32 &rb, const BRegion& region, uint32 color)
{
	for (int32 i = 0; i < region.CountRects(); i++) {
		clipping_rect rect = region.RectAtInt(i);
		rb.Clip2(rect.left, rect.top, rect.right + 1, rect.bottom + 1).Clear(color);
	}
}

class ProducerApp {
public:
	class VideoProducer: public ::VideoProducer {
	public:
		int32 fWidth, fHeight;
		bool fSuboptimal = false;

		using ::VideoProducer::VideoProducer;
		virtual ~VideoProducer() {}

		void Presented(const PresentedInfo &presentedInfo) final
		{
			if (presentedInfo.suboptimal) {
				fSuboptimal = true;
				fWidth = presentedInfo.width;
				fHeight = presentedInfo.height;
			}
		}
	};
	VideoProducer *fProducer;
	BLooper *fProducerLooper;
	SwapChainBindSW fSwapChainBind;

	bigtime_t fStartTime;
	uint32 fSequence;
	BRect fRect;


	RasBuf32 GetRasBuf(int32 bufferId)
	{
		if (bufferId < 0) {
			RasBuf32 rb {};
			return rb;
		}
		const VideoBuffer &buf = AppKitPtrs::LockedPtr(fProducer)->GetSwapChain().buffers[bufferId];
		const auto &mappedBuffer = fSwapChainBind.Buffers()[bufferId];
		RasBuf32 rb = {
			.colors = (uint32*)mappedBuffer.bits,
			.stride = buf.format.bytesPerRow / 4,
			.width  = buf.format.width,
			.height = buf.format.height,
		};
		return rb;
	}

	void Layout(int32 bufferId)
	{
		static float minX = 32;
		static float minY = 32;
		static float period = 12;

		const VideoBuffer& buf = AppKitPtrs::LockedPtr(fProducer)->GetSwapChain().buffers[bufferId];
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

	void Prepare(int32 bufferId, BRegion& dirty)
	{
		dirty.Include(fRect);
		Layout(bufferId);
		dirty.Include(fRect);
	}

	void Restore(int32 bufferId, const BRegion& dirty)
	{
		RasBuf32 rb = GetRasBuf(bufferId);
		FillRegion(rb, dirty, 0x80cccccc);
		BRegion region = BRegion(fRect);
		region.IntersectWith(&dirty);
		FillRegion(rb, region, 0x80000000);
		fSequence++;
	}

	void UpdateSwapChain(int32 width, int32 height)
	{
		bool ownsSwapChain = true;
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
			if (AppKitPtrs::LockedPtr(fProducer)->RequestSwapChain(spec) < B_OK) {
				printf("[!] can't request swap chain\n");
				abort();
			}
		} else {
			ObjectDeleter<SwapChain> swapChain;
			fSwapChainBind.Alloc(swapChain, spec);
			DumpSwapChain(*swapChain.Get());
			AppKitPtrs::LockedPtr(fProducer)->SetSwapChain(swapChain.Get());
		}
	}

	void Init(const BMessenger &consumer)
	{
		fProducer = new VideoProducer("producer");
		fProducerLooper = new BLooper("producer");
		fProducerLooper->AddHandler(fProducer);
		fProducerLooper->Run();

		if (AppKitPtrs::LockedPtr(fProducer)->ConnectTo(consumer) < B_OK) {
			printf("[!] can't connect to consumer\n");
			exit(1);
		}

		UpdateSwapChain(640, 480);
		snooze(10000); // !!!

		fStartTime = system_time();
	}

	~ProducerApp()
	{
		AppKitPtrs::LockedPtr(fProducer)->ConnectTo(BMessenger());
		fProducerLooper->Quit();
		delete fProducer;
	}

	void Draw(int32 bufferId)
	{
		const VideoBuffer& buf = AppKitPtrs::LockedPtr(fProducer)->GetSwapChain().buffers[bufferId];
		BRegion dirty(BRect(0, 0, buf.format.width - 1, buf.format.height - 1));
		Layout(bufferId);
		Restore(bufferId, dirty);
	}

	void RunLoop()
	{
		bool quit = false;
		while (!quit) {
			int32 bufferId = AppKitPtrs::LockedPtr(fProducer)->AllocBufferWait();
			Draw(bufferId);
			AppKitPtrs::LockedPtr(fProducer)->Present(bufferId);
			auto producer = AppKitPtrs::LockedPtr(fProducer);
			if (producer->fSuboptimal) {
				producer->fSuboptimal = false;
				UpdateSwapChain(producer->fWidth, producer->fHeight);
			}
		}
	}
};


class TestApplication: public BApplication {
public:
	using BApplication::BApplication;
	virtual ~TestApplication() {}

	thread_id Run() {
		return BLooper::Run();
	}

	void Quit() {
		BLooper::Quit();
	}

};


int main()
{
	TestApplication *app = new TestApplication("application/x-vnd.VideoStreams-TestProducer");
	app->Run();

	BMessenger consumer;
	while (!FindConsumer(consumer)) {
		snooze(100000);
	}
	printf("consumer: "); WriteMessenger(consumer); printf("\n");

	ProducerApp producerApp;
	producerApp.Init(consumer);
	producerApp.RunLoop();
	return 0;
}
