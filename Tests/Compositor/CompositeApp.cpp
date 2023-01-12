#include <Application.h>
#include <Messenger.h>
#include <stdio.h>
#include <string.h>
#include <private/shared/AutoDeleter.h>

#include <VideoProducer.h>
#include <CompositeProducer.h>
#include <CompositeProducerSoftware.h>

#include <FindConsumer.h>


class TestApplication: public BApplication
{
private:
	ObjectDeleter<CompositeProducer> fProducer;
	BReference<Accelerant> fAcc;

public:
	TestApplication(): BApplication("application/x-vnd.VideoStreams-Compositor")
	{
		BMessenger consumer;
		while (!FindConsumer(consumer)) {
			snooze(100000);
		}
		printf("consumer: "); WriteMessenger(consumer); printf("\n");
		fProducer.SetTo(new CompositeProducerSoftware("compositeProducer"));
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
