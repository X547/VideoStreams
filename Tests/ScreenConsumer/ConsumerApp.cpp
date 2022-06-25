#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <WindowScreen.h>
#include <Screen.h>
#include <View.h>
#include <Rect.h>
#include <Region.h>
#include <Bitmap.h>
#include <OS.h>

#include <private/shared/AutoDeleter.h>

#include <VideoConsumer.h>
#include <VideoBuffer.h>


#define CheckRet(err) {status_t _err = (err); if (_err < B_OK) return _err;}


class TestConsumer: public VideoConsumer
{
private:
	BView* fView;

	inline BWindowScreen* Wnd() {return (BWindowScreen*)fView->Window();}

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
		}
	}
	
	status_t SwapChainRequested(const SwapChainSpec& spec)
	{
		printf("TestConsumer::SwapChainRequested(%" B_PRIuSIZE ")\n", spec.bufferCnt);
		
		area_id framebufferArea = area_for(Wnd()->CardInfo()->frame_buffer);
		CheckRet(framebufferArea);
		area_info areaInfo;
		CheckRet(get_area_info(framebufferArea, &areaInfo));
		void* framebufferAreaBase = areaInfo.address;
		
		SwapChain swapChain;
		swapChain.size = sizeof(SwapChain);
		swapChain.presentEffect = presentEffectSwap;
		swapChain.bufferCnt = 2;
		ArrayDeleter<VideoBuffer> buffers(new VideoBuffer[spec.bufferCnt]);
		swapChain.buffers = buffers.Get();
		for (uint32 i = 0; i < swapChain.bufferCnt; i++) {
			buffers[i].id = i;
			buffers[i].ref.kind           = bufferRefArea;
			buffers[i].ref.area.id        = framebufferArea;
			buffers[i].ref.offset         = (addr_t)Wnd()->CardInfo()->frame_buffer - (addr_t)framebufferAreaBase +
				i*Wnd()->FrameBufferInfo()->bytes_per_row * Wnd()->FrameBufferInfo()->display_height;
			buffers[i].ref.size           = Wnd()->FrameBufferInfo()->bytes_per_row * Wnd()->FrameBufferInfo()->display_height;
			buffers[i].format.bytesPerRow = Wnd()->FrameBufferInfo()->bytes_per_row;
			buffers[i].format.width       = Wnd()->FrameBufferInfo()->display_width;
			buffers[i].format.height      = Wnd()->FrameBufferInfo()->display_height;
			buffers[i].format.colorSpace  = B_RGBA32;
		}
		SetSwapChain(&swapChain);
		return B_OK;
	}

	void Present(int32 bufferId, const BRegion* dirty) final
	{
		int32 height = Wnd()->FrameBufferInfo()->height/2;
		Wnd()->MoveDisplayArea(0, height*bufferId);
		PresentedInfo presentedInfo {};
		Presented(presentedInfo);
	}

};

class TestView: public BView
{
private:
	ObjectDeleter<TestConsumer> fConsumer;

public:
	TestView(BRect frame, const char *name): BView(frame, name, B_FOLLOW_NONE, 0)
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

};

class TestWindow: public BWindowScreen
{
private:
	status_t fError;
	int32 (*fDrawRect32)(int32 sx, int32 sy, int32 sw, int32 sh, uint32 color);
	TestView *fView;

public:
	TestWindow(): BWindowScreen("ScreenConsumer", B_32_BIT_640x480, &fError, false)
	{
		printf("error: %s\n", strerror(fError));
		SetFlags(Flags() | B_CLOSE_ON_ESCAPE);

		fView = new TestView(BScreen().Frame().OffsetToCopy(B_ORIGIN), "view");
		fView->SetResizingMode(B_FOLLOW_ALL);
		AddChild(fView, NULL);
	}
	
	void ScreenConnected(bool connected)
	{
		printf("ScreenConnected(%s)\n", connected? "true": "false");
		if (!connected)
			return;
		printf("CanControlFrameBuffer(): %d\n", CanControlFrameBuffer());
		
		SetFrameBuffer(640, 480*2);
		
		//*(graphics_card_hook*)&fDrawRect32 = CardHookAt(6);
		//printf("fDrawRect32: %p\n", fDrawRect32);

		//fDrawRect32(0, 0, 640 - 1, 480 - 1, 0xffffffff);

		//fDrawRect32(16, 16, 320 - 1, 240 - 1, 0xff0099ff);
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
		fWnd = new TestWindow();
		fWnd->Show();
	}
};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
