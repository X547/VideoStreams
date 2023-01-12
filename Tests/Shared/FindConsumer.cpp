#include <Messenger.h>
#include <AccelerantRoster.h>
#include <AutoDeleterPosix.h>

#include <stdio.h>
#include <string.h>


bool FindConsumer(BMessenger& consumer)
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

bool FindConsumerGfx(BReference<Accelerant> &accRef, BMessenger& consumer)
{
	FileDescriptorCloser fd(open("/dev/graphics/radeon_gfx_010000", B_READ_WRITE | O_CLOEXEC));
	if (!fd.IsSet()) return false;
	Accelerant *acc = NULL;
	if (AccelerantRoster::Instance().FromFd(acc, fd.Get()) < B_OK) return false;
	accRef.SetTo(acc, true);
	AccelerantDisplay *accDisp = dynamic_cast<AccelerantDisplay*>(acc);
	if (accDisp == NULL) return false;
	if (accDisp->DisplayGetConsumer(0, consumer) < B_OK) return false;
	return true;
}
