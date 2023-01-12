#include <Application.h>
#include <Messenger.h>
#include <stdio.h>
#include <string.h>
#include <private/shared/AutoDeleter.h>
#include <private/shared/AutoDeleterOS.h>

#include <FindConsumer.h>
#include "TestProducer.h"


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
	}
};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
