#ifndef _CONSUMERVIEW_H_
#define _CONSUMERVIEW_H_

#include <View.h>

#include <VideoConsumer.h>


class ViewConsumer final: public VideoConsumer
{
private:
	friend class ConsumerView;

	BView* fView;
	ArrayDeleter<ObjectDeleter<BBitmap> > fBitmaps;

	status_t SetupSwapChain();
	inline BBitmap* DisplayBitmap();

public:
	ViewConsumer(const char* name, BView* view);
	virtual ~ViewConsumer();
	
	void Connected(bool isActive) final;
	status_t SwapChainRequested(const SwapChainSpec& spec) final;
	void Present(int32 bufferId, const BRegion* dirty) final;
};


class ConsumerView: public BView
{
private:
	ObjectDeleter<ViewConsumer> fConsumer;

public:
	using BView::BView;

	void AttachedToWindow() override;
	void DetachedFromWindow() override;
	void FrameResized(float newWidth, float newHeight) override;
	void Draw(BRect dirty) override;

	inline VideoConsumer* GetConsumer();
};


BBitmap* ViewConsumer::DisplayBitmap()
{
	int32 bufferId = DisplayBufferId();
	if (bufferId < 0) {return NULL;}
	return fBitmaps[bufferId].Get();
}


VideoConsumer* ConsumerView::GetConsumer()
{
	return fConsumer.Get();
}


#endif	// _CONSUMERVIEW_H_
