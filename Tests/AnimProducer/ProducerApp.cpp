#include <Application.h>
#include <Messenger.h>
#include <stdio.h>
#include <string.h>
#include <private/shared/AutoDeleter.h>
#include <private/shared/AutoDeleterOS.h>

#include <VideoProducer.h>
#include "TestProducerBase.h"
#include "AnimProducer.h"
#include "CompositeProducer.h"
#include "CompositeProxy.h"


inline void Check(status_t res) {if (res < B_OK) {fprintf(stderr, "Error: %s\n", strerror(res)); abort();}}


static bool FindCompositor(BMessenger& compositor)
{
	BMessenger consumerApp("application/x-vnd.VideoStreams-Compositor");
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
		if (reply.FindMessenger("result", &compositor) >= B_OK) {
			BMessage scriptMsg(B_GET_PROPERTY);
			scriptMsg.AddSpecifier("InternalName");
			compositor.SendMessage(&scriptMsg, &reply);
			const char* name;
			if (reply.FindString("result", &name) >= B_OK && strcmp(name, "compositeProducer") == 0)
				return true;
		}
	}
}


class TestApplication: public BApplication
{
private:
	ObjectDeleter<CompositeProducer> fProducer;
	int fCount;
	ObjectDeleter<CompositeProxy> fCompositor;
	ArrayDeleter<AnimProducer*> fProducers;
	ArrayDeleter<BMessenger> fCompositeConsumers;

public:
	TestApplication(): BApplication("application/x-vnd.VideoStreams-TestProducer")
	{
		BMessenger compositeProducer;
		while (!FindCompositor(compositeProducer)) {
			snooze(100000);
		}
		printf("compositeProducer: "); WriteMessenger(compositeProducer); printf("\n");
		fCompositor.SetTo(new CompositeProxy(compositeProducer));

		fCount = 1;
		fProducers.SetTo(new AnimProducer*[fCount]);
		fCompositeConsumers.SetTo(new BMessenger[fCount]);
		for (int i = 0; i < fCount; i++) {
			fProducers[i] = new AnimProducer("testProducer2");
			AddHandler(fProducers[i]);

			SurfaceUpdate update = {
				.valid = (1 << surfaceFrame) | (1 << surfaceDrawMode),
				.frame = fProducers[i]->Bounds().OffsetByCopy(32 + 64*i, 32 + 48*i),
				.drawMode = B_OP_COPY
			};

			if (false) {
				BRegion clipping;
				clipping.Include(fProducers[i]->Bounds());
				clipping.Exclude(BRect(128, 128, 192, 192));
				clipping.Exclude(BRect(128, 128, 192, 192).OffsetByCopy(-32, 32));
				update.valid |= (1 << surfaceClipping);
				update.clipping = &clipping;
			}

			Check(fCompositor->NewSurface(fCompositeConsumers[i], "surface", update));
			fProducers[i]->SetSurface(compositeProducer, fCompositeConsumers[i]);
			Check(fProducers[i]->ConnectTo(fCompositeConsumers[i]));
		}
	}
	
	~TestApplication()
	{
		for (int i = 0; i < fCount; i++) {
			fProducers[i]->ConnectTo(BMessenger());
			Check(fCompositor->DeleteSurface(fCompositeConsumers[i]));
			delete fProducers[i];
		}
	}
};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
