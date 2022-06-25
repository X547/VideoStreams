#include <Application.h>
#include <Messenger.h>
#include <stdio.h>
#include <string.h>
#include <private/shared/AutoDeleter.h>
#include <private/shared/AutoDeleterOS.h>

#include <VideoProducer.h>
#include <VideoBufferBindSW.h>
#include "AppKitPtrs.h"


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

static bool FindConsumerGfx(BMessenger& consumer)
{
	BMessenger consumerApp("application/x-vnd.X512-RadeonGfx");
	if (!consumerApp.IsValid()) {
		printf("[!] No TestConsumer\n");
		return false;
	}
	for (int32 i = 0; ; i++) {
		BMessage reply;
		{
			BMessage scriptMsg(B_GET_PROPERTY);
			scriptMsg.AddSpecifier("Handler", i);
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
			if (reply.FindString("result", &name) >= B_OK && strcmp(name, "RadeonGfxConsumer") == 0)
				return true;
		}
	}
}



class ProducerApp {
public:
	VideoProducer *fProducer;
	BLooper *fProducerLooper;
	SwapChainBindSW fSwapChainBind;

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

		SwapChainSpec spec {
			.size = sizeof(SwapChainSpec),
			.presentEffect = presentEffectSwap,
			.bufferCnt = 2,
			.kind = bufferRefArea,
			.width = 640,
			.height = 480,
			.colorSpace = B_RGBA32
		};
		if (AppKitPtrs::LockedPtr(fProducer)->RequestSwapChain(spec) < B_OK) {
			printf("[!] can't request swap chain\n");
			abort();
		}
	}

	~ProducerApp()
	{
		AppKitPtrs::LockedPtr(fProducer)->ConnectTo(BMessenger());
		fProducerLooper->Quit();
		delete fProducer;
	}

	void Draw(int32 bufferId)
	{
	}

	void RunLoop()
	{
		bool quit = false;
		while (!quit) {
			int32 bufferId = -1;
			for (;;) {
				bufferId = AppKitPtrs::LockedPtr(fProducer)->AllocBuffer();
				if (bufferId >= 0) break;
				snooze(1000);
			}
			Draw(bufferId);
			AppKitPtrs::LockedPtr(fProducer)->Present(bufferId);
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
