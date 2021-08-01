#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Rect.h>
#include <Bitmap.h>
#include <OS.h>
#include <stdio.h>
#include <private/shared/AutoDeleter.h>

#include <VideoConsumer.h>


class TestConsumer: public VideoConsumer
{
private:
	BView* fView;
	ArrayDeleter<ObjectDeleter<BBitmap> > fBitmaps;

public:
	TestConsumer(const char* name, BView* view): VideoConsumer(name), fView(view)
	{
	}

	virtual ~TestConsumer()
	{
		printf("-TestConsumer: "); WriteMessenger(BMessenger(this)); printf("\n");
	}

	void Connected(bool isActive)
	{
		if (isActive) {
			printf("TestConsumer: connected to ");
			WriteMessenger(Link());
			printf("\n");
		} else {
			printf("TestConsumer: disconnected\n");
			SetSwapChain(NULL);
			fBitmaps.Unset();
			fView->Invalidate();
		}
	}
	
	status_t SwapChainRequested(const SwapChainSpec& spec)
	{
		printf("TestConsumer::SwapChainRequested(%" B_PRIu32 ")\n", spec.bufferCnt);
		fBitmaps.SetTo(new ObjectDeleter<BBitmap>[spec.bufferCnt]);
		for (uint32 i = 0; i < spec.bufferCnt; i++) {
			fBitmaps[i].SetTo(new BBitmap(BRect(0, 0, 255, 255), spec.bufferSpecs[i].colorSpace));
		}
		SwapChain swapChain;
		swapChain.size = sizeof(SwapChain);
		swapChain.bufferCnt = spec.bufferCnt;
		ArrayDeleter<VideoBuffer> buffers(new VideoBuffer[spec.bufferCnt]);
		swapChain.buffers = buffers.Get();
		for (uint32 i = 0; i < spec.bufferCnt; i++) {
			area_info info;
			get_area_info(fBitmaps[i]->Area(), &info);
			buffers[i].id = i;
			buffers[i].area        = fBitmaps[i]->Area();
			buffers[i].offset      = (addr_t)fBitmaps[i]->Bits() - (addr_t)info.address;
			buffers[i].length      = fBitmaps[i]->BitsLength();
			buffers[i].bytesPerRow = fBitmaps[i]->BytesPerRow();
			buffers[i].width       = fBitmaps[i]->Bounds().Width() + 1;
			buffers[i].height      = fBitmaps[i]->Bounds().Height() + 1;
			buffers[i].colorSpace  = fBitmaps[i]->ColorSpace();
		}
		SetSwapChain(&swapChain);
		return B_OK;
	}

	virtual void Present(const BRegion* dirty)
	{
		// printf("TestConsumer::Present()\n");
		fView->Invalidate();
		Presented();
	}

	BBitmap* DisplayBitmap()
	{
		int32 bufferId = DisplayBufferId();
		if (bufferId < 0)
			return NULL;
		return fBitmaps[bufferId].Get();
	}
	
};

class TestView: public BView
{
private:
	ObjectDeleter<TestConsumer> fConsumer;

public:
	TestView(BRect frame, const char *name): BView(frame, name, B_FOLLOW_NONE, B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW | B_SUBPIXEL_PRECISE)
	{
		SetViewColor(0xff - 0x44, 0xff - 0x44, 0xff - 0x44);
	}
	
	void AttachedToWindow()
	{
		fConsumer.SetTo(new TestConsumer("testConsumer", this));
		Looper()->AddHandler(fConsumer.Get());
		printf("+TestConsumer: "); WriteMessenger(BMessenger(fConsumer.Get())); printf("\n");
	}

	void DetachedFromWindow()
	{
		fConsumer.Unset();
	}

	void Draw(BRect dirty)
	{
		BBitmap* bmp = fConsumer->DisplayBitmap();
		if (bmp != NULL) {
			DrawBitmap(bmp);
			return;
		}
		BRect rect = Frame().OffsetToCopy(B_ORIGIN);
		rect.left += 1; rect.top += 1;
		PushState();
		SetHighColor(0x44, 0x44, 0x44);
		SetPenSize(2);
		StrokeRect(rect, B_SOLID_HIGH);
		SetPenSize(1);
		StrokeLine(rect.LeftTop(), rect.RightBottom());
		StrokeLine(rect.RightTop(), rect.LeftBottom());
		PopState();
	}
};

class TestWindow: public BWindow
{
private:
	TestView *fView;
public:
	TestWindow(BRect frame): BWindow(frame, "TestConsumer", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
	{
		fView = new TestView(frame.OffsetToCopy(B_ORIGIN), "view");
		fView->SetResizingMode(B_FOLLOW_ALL);
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
