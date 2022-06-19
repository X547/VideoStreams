#ifndef _CONSUMERVIEW_H_
#define _CONSUMERVIEW_H_

#include <View.h>

#include <VideoConsumer.h>

class ViewConsumer;


class _EXPORT ConsumerView: public BView
{
private:
	ObjectDeleter<ViewConsumer> fConsumer;

public:
	using BView::BView;

	void AttachedToWindow() override;
	void DetachedFromWindow() override;
	void FrameResized(float newWidth, float newHeight) override;
	void Draw(BRect dirty) override;

	VideoConsumer *GetConsumer();
};


#endif	// _CONSUMERVIEW_H_
