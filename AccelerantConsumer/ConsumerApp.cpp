#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Window.h>
#include <Screen.h>
#include <View.h>
#include <Cursor.h>

#include "AccelerantConsumer.h"


#define CheckRet(err) {status_t _err = (err); if (_err < B_OK) return _err;}


class TestWindow final: public BWindow
{
private:
	ObjectDeleter<AccelerantConsumer> fConsumer;

public:
	TestWindow(): BWindow(BRect(0, 0, 32, 32), "AccelerantConsumer", B_NO_BORDER_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS)
	{
	
		SetFlags(Flags() | B_CLOSE_ON_ESCAPE);

		fConsumer.SetTo(new AccelerantConsumer("testConsumer"));
		if (fConsumer->Init() < B_OK) {
			be_app_messenger.SendMessage(B_QUIT_REQUESTED);
			return;
		}

		Looper()->AddHandler(fConsumer.Get());
		BScreen screen(this);
		BRect screenFrame = screen.Frame();
		MoveTo(screenFrame.LeftTop());
		ResizeTo(screenFrame.Width(), screenFrame.Height());
/*
		BView *view = new BView(screenFrame.OffsetToCopy(B_ORIGIN), "view", B_FOLLOW_ALL, 0);
		AddChild(view);
		BCursor cursor(B_CURSOR_ID_NO_CURSOR);
		view->SetViewCursor(&cursor, true);
*/
		//be_app->HideCursor();
	}

	void Quit() final
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
		fWnd = new TestWindow();
		fWnd->Run();
	}
};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
