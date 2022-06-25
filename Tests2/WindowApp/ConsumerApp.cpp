#include <Application.h>
#include <Window.h>
#include <stdio.h>

#include <ConsumerView.h>
#include <VideoConsumer.h>

#include "TestProducer.h"
#include "TestFilter.h"

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


class TestWindow: public BWindow
{
private:
	friend class TestApplication;

	ConsumerView *fView;
	TestProducer *fProducer;
	ObjectDeleter<TestProducer> fProducerDeleter;
	TestFilter fFilter;

public:
	TestWindow(BRect frame): BWindow(frame, "TestConsumer", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
	{
		fView = new ConsumerView(frame.OffsetToCopy(B_ORIGIN), "view", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS);
		AddChild(fView, NULL);

		fProducer = new TestProducer("TestProducer");
	}

	void Quit()
	{
		be_app_messenger.SendMessage(B_QUIT_REQUESTED);
		BWindow::Quit();
	}

};

class TestApplication: public BApplication
{
private:
	TestWindow *fWnd;

public:
	TestApplication(): BApplication("application/x-vnd.VideoStreams-WindowApp")
	{
	}

	void ReadyToRun() {
		fWnd = new TestWindow(BRect(0, 0, 640 - 1, 480 - 1).OffsetByCopy(64, 64));
		fWnd->Run();

		switch (0) {
			case 0: {
				// producer (wnd) -> consumer (remote)
				BMessenger consumer;
				if (!FindConsumer(consumer)) abort();
				AppKitPtrs::LockedPtr(fWnd)->AddHandler(fWnd->fProducer);
				AppKitPtrs::LockedPtr(fWnd)->fProducerDeleter.SetTo(fWnd->fProducer);
				AppKitPtrs::LockedPtr(fWnd->fProducer)->ConnectTo(consumer);
				break;
			}
			case 1: {
				// producer (wnd) -> consumer (wnd)
				AppKitPtrs::LockedPtr(fWnd)->AddHandler(fWnd->fProducer);
				AppKitPtrs::LockedPtr(fWnd)->fProducerDeleter.SetTo(fWnd->fProducer);
				AppKitPtrs::LockedPtr(fWnd->fProducer)->ConnectTo(BMessenger(fWnd->fView->GetConsumer()));
				break;
			}
			case 2: {
				// producer (looper1) -> filter (looper2) -> consumer (wnd)
				BLooper *looper1 = new BLooper("producer");
				looper1->AddHandler(fWnd->fProducer);
				looper1->Run();

				BLooper *looper2 = new BLooper("filter");
				looper2->AddHandler(fWnd->fFilter.GetConsumer());
				looper2->AddHandler(fWnd->fFilter.GetProducer());
				looper2->Run();

				AppKitPtrs::LockedPtr(fWnd->fProducer)->ConnectTo(BMessenger(fWnd->fFilter.GetConsumer()));
				AppKitPtrs::LockedPtr(fWnd->fFilter.GetProducer())->ConnectTo(BMessenger(fWnd->fView->GetConsumer()));
				break;
			}
		}

		fWnd->Show();
	}
};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
