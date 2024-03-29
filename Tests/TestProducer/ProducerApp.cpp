#include <Application.h>
#include <Messenger.h>
#include <stdio.h>
#include <string.h>
#include <private/shared/AutoDeleter.h>
#include <private/shared/AutoDeleterOS.h>

#include <VideoProducer.h>
#include "TestProducerBase.h"
#include "TestProducer.h"
#include "AnimProducer.h"
#include "CompositeProducer.h"

#include <FindConsumer.h>


class TestApplication: public BApplication
{
private:
	ObjectDeleter<TestProducer> fProducer;

public:
	TestApplication(): BApplication("application/x-vnd.VideoStreams-TestProducer")
	{
		BMessenger consumer;
		while (!FindConsumer(consumer)) {
			snooze(100000);
		}
		printf("consumer: "); WriteMessenger(consumer); printf("\n");
		fProducer.SetTo(new TestProducer("testProducer"));
		AddHandler(fProducer.Get());
		printf("+TestProducer: "); WriteMessenger(BMessenger(fProducer.Get())); printf("\n");

		if (fProducer->ConnectTo(consumer) < B_OK) {
			printf("[!] can't connect to consumer\n");
			exit(1);
		}
/*
		for (int i = 0; i < 3; i++) {
			AnimProducer* testProducer = new AnimProducer("testProducer2");
			AddHandler(testProducer);
			BRegion clipping;
			clipping.Include(testProducer->Bounds());
			clipping.Exclude(BRect(128, 128, 192, 192));
			clipping.Exclude(BRect(128, 128, 192, 192).OffsetByCopy(-32, 32));
			SurfaceUpdate update = {
				.valid = (1 << surfaceFrame) | (1 << surfaceClipping) | (1 << surfaceDrawMode),
				.frame = testProducer->Bounds().OffsetByCopy(32 + 64*i, 32 + 48*i),
				.clipping = &clipping,
				.drawMode = B_OP_ALPHA
			};
			CompositeConsumer* compositeConsumer = fProducer->NewSurface("surface", update);
			if (testProducer->ConnectTo(BMessenger((BHandler*)compositeConsumer))) {
				printf("[!] can't connect to CompositeConsumer\n");
			}
		}
*/
	}
};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
