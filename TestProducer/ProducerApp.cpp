#include <Application.h>
#include <Messenger.h>
#include <MessageRunner.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <private/shared/AutoDeleter.h>
#include <private/shared/AutoDeleterOS.h>

#include <map>

#include <VideoProducer.h>


static bool FindConsumer(BMessenger& consumer)
{
	BMessenger consumerApp("application/x-vnd.VideoStreams-TestConsumer");
	if (!consumerApp.IsValid()) {
		printf("[!] No TestConsumer\n");
		return false;
	}
	for (int32 i = 0; ; i++) {
		BMessage reply;
		{
			BMessage scriptMsg(B_GET_PROPERTY);
			scriptMsg.AddSpecifier("Handler", i);
			scriptMsg.AddSpecifier("Window", (int32)0);
			consumerApp.SendMessage(&scriptMsg, &reply);
		}
		int32 error;
		if (reply.FindInt32("error", &error) >= B_OK && error < B_OK)
			return false;
		if (reply.FindMessenger("result", &consumer) >= B_OK) {
			BMessage scriptMsg(B_GET_PROPERTY);
			scriptMsg.AddSpecifier("InternalName");
			consumer.SendMessage(&scriptMsg, &reply);
			const char* name;
			if (reply.FindString("result", &name) >= B_OK && strcmp(name, "testConsumer") == 0)
				return true;
		}
	}
}


struct MappedBuffer
{
	area_id area;
	uint8* bits;
};

struct MappedArea
{
	AreaDeleter area;
	uint8* adr;

	MappedArea(area_id srcArea):
		area(clone_area("cloned buffer", (void**)&adr, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, srcArea))
	{}
};


class TestProducer: public VideoProducer
{
private:
	enum {
		stepMsg = videoNodeLastMsg + 1,
	};

	ArrayDeleter<MappedBuffer> fMappedBuffers;
	std::map<area_id, MappedArea> fMappedAreas;
	ObjectDeleter<BMessageRunner> fMessageRunner;
	uint32 fSequence;
	
	void Produce()
	{
		enum {
			minX = 32,
			minY = 32,
			maxX = 256 - 32,
			maxY = 256 - 32,
			period = 2
		};

		static const float stepDelay = 1/60.0;
		// printf("renderBufferId: %" B_PRId32 "\n", RenderBufferId());
		// printf("fSequence: %" B_PRIu32 "\n", fSequence);

		const VideoBuffer& buf = *RenderBuffer();
		uint32* colors = (uint32*)fMappedBuffers[RenderBufferId()].bits;
		int32 stride = buf.bytesPerRow / 4;

		float fTime;
		BPoint fPos;
		
		fTime = fSequence*stepDelay;
		float a = 2*M_PI*fTime/period;
		fPos.x = minX + (maxX - minX)*(cosf(a) + 1)/2;
		fPos.y = minY + (maxY - minY)*(sinf(2*a) + 1)/2;
	
		for (int32 y = 0; y < buf.height; y++)
			for (int32 x = 0; x < buf.width; x++) {
				colors[y*stride + x] = 0x80ffffff;
			}
		
		for (int32 y = std::max<int32>(fPos.y - 16, 0); y < std::min<int32>(fPos.y + 16, buf.height); y++)
			for (int32 x = std::max<int32>(fPos.x - 16, 0); x < std::min<int32>(fPos.x + 16, buf.width); x++) {
				colors[y*stride + x] = 0x80000000;
			}

		fSequence++;
		Present();
	}
	
public:
	TestProducer(const char* name):
		VideoProducer(name),
		fSequence(0)
	{
	}

	virtual ~TestProducer()
	{
		printf("-TestProducer: "); WriteMessenger(BMessenger(this)); printf("\n");
	}

	void Connected(bool isActive) final
	{
		if (isActive) {
			printf("TestProducer: connected to ");
			WriteMessenger(Link());
			printf("\n");
			fSequence = 0;
		} else {
			printf("TestProducer: disconnected\n");
			be_app_messenger.SendMessage(B_QUIT_REQUESTED);
		}
	}
	
	void SwapChainChanged(bool isValid) final
	{
		VideoProducer::SwapChainChanged(isValid);
		printf("TestProducer::SwapChainChanged(%d)\n", isValid);

		fMappedAreas.clear();
		fMappedBuffers.Unset();

		if (isValid) {
			printf("  swapChain: \n");
			printf("    size: %" B_PRIuSIZE "\n", GetSwapChain().size);
			printf("    bufferCnt: %" B_PRIu32 "\n", GetSwapChain().bufferCnt);
			printf("    buffers:\n");
			for (uint32 i = 0; i < GetSwapChain().bufferCnt; i++) {
				printf("      %" B_PRIu32 "\n", i);
				printf("        area: %" B_PRId32 "\n", GetSwapChain().buffers[i].area);
				printf("        offset: %" B_PRIuSIZE "\n", GetSwapChain().buffers[i].offset);
				printf("        length: %" B_PRIu32 "\n", GetSwapChain().buffers[i].length);
				printf("        bytesPerRow: %" B_PRIu32 "\n", GetSwapChain().buffers[i].bytesPerRow);
				printf("        width: %" B_PRIu32 "\n", GetSwapChain().buffers[i].width);
				printf("        height: %" B_PRIu32 "\n", GetSwapChain().buffers[i].height);
				printf("        colorSpace: %d\n", GetSwapChain().buffers[i].colorSpace);
			}

			fMappedBuffers.SetTo(new MappedBuffer[GetSwapChain().bufferCnt]);
			for (uint32 i = 0; i < GetSwapChain().bufferCnt; i++) {
				auto it = fMappedAreas.find(GetSwapChain().buffers[i].area);
				if (it == fMappedAreas.end())
					it = fMappedAreas.emplace(GetSwapChain().buffers[i].area, GetSwapChain().buffers[i].area).first;
	
				const MappedArea& mappedArea = (*it).second;
				fMappedBuffers[i].bits = mappedArea.adr + GetSwapChain().buffers[i].offset;
			}
			
			Produce();
		}
	}
	
	void Presented() final
	{
		// printf("TestProducer::Presented()\n");
		fMessageRunner.SetTo(new BMessageRunner(BMessenger(this), BMessage(stepMsg), 1000000/60, 1));
	}
	
	void MessageReceived(BMessage* msg) final
	{
		switch (msg->what) {
		case stepMsg: {
			Produce();
			return;
		}
		}
		VideoProducer::MessageReceived(msg);
	};
	
};


class TestApplication: public BApplication
{
private:
	ObjectDeleter<TestProducer> fProducer;

public:
	TestApplication(): BApplication("application/x-vnd.VideoStreams-TestProducer")
	{
		BMessenger consumer;
		if (!FindConsumer(consumer)) {
			printf("[!] no TestConsumer\n");
			exit(1);
		}
		printf("consumer: "); WriteMessenger(consumer); printf("\n");
		fProducer.SetTo(new TestProducer("testProducer"));
		AddHandler(fProducer.Get());
		printf("+TestProducer: "); WriteMessenger(BMessenger(fProducer.Get())); printf("\n");

		if (fProducer->ConnectTo(consumer) < B_OK) {
			printf("[!] can't connect to consumer\n");
			exit(1);
		}


		SwapChainSpec spec;
		BufferSpec buffers[2];
		spec.size = sizeof(SwapChainSpec);
		spec.presentEffect = presentEffectSwap;
		spec.bufferCnt = 2;
		spec.bufferSpecs = buffers;
		for (uint32 i = 0; i < spec.bufferCnt; i++) {
			buffers[i].colorSpace = B_RGBA32;
		}
		if (fProducer->RequestSwapChain(spec) < B_OK) {
			printf("[!] can't request swap chain\n");
			exit(1);
		}
	}
};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
