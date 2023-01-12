#include <Application.h>
#include <Window.h>
#include <stdio.h>
#include "ConsumerGpuView.h"

#include <VideoConsumer.h>


class TestWindow: public BWindow
{
private:
	ConsumerGpuView *fView;
public:
	TestWindow(BRect frame): BWindow(frame, "TestConsumer", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
	{
		fView = new ConsumerGpuView(frame.OffsetToCopy(B_ORIGIN), "view", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS);
		AddChild(fView, NULL);
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
	TestApplication(): BApplication("application/x-vnd.VideoStreams-TestConsumer")
	{
	}

	void ReadyToRun() {
		fWnd = new TestWindow(BRect(0, 0, 255, 255).OffsetByCopy(64, 64));
		fWnd->Show();
	}
};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
