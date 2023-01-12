#include <Application.h>
#include <Messenger.h>
#include <stdio.h>
#include <string.h>
#include <private/shared/AutoDeleter.h>
#include <private/shared/AutoDeleterOS.h>

#include <VideoProducer.h>
#include "TestProducerBase.h"
#include "VideoFileProducer.h"

#include <FindConsumer.h>


class TestApplication: public BApplication
{
private:
	ObjectDeleter<VideoFileProducer> fProducer;
	BReference<Accelerant> fAcc;

public:
	TestApplication(): BApplication("application/x-vnd.VideoStreams-TestProducer")
	{
		BMessenger consumer;
		while (!FindConsumerGfx(fAcc, consumer)) {
			snooze(100000);
		}
		printf("consumer: "); WriteMessenger(consumer); printf("\n");
		fProducer.SetTo(new VideoFileProducer("testProducer"));
		AddHandler(fProducer.Get());
		printf("+VideoFileProducer: "); WriteMessenger(BMessenger(fProducer.Get())); printf("\n");

		if (fProducer->ConnectTo(consumer) < B_OK) {
			printf("[!] can't connect to consumer\n");
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
