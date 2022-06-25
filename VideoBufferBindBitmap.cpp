#include "VideoBufferBindBitmap.h"
#include "VideoBuffer.h"
#include "VideoBufferUtils.h"

#define CheckRet(err) {status_t _err = (err); if (_err < B_OK) return _err;}


void SwapChainBindBitmap::Unset()
{
	fBindedBuffers.Unset();
}

status_t SwapChainBindBitmap::ConnectTo(const SwapChain &swapChain)
{
	fBindedBuffers.SetTo(new BindedBuffer[swapChain.bufferCnt]);
	for (uint32 i = 0; i < swapChain.bufferCnt; i++) {
		const auto &buffer = swapChain.buffers[i];
		auto &bindedBuffer = fBindedBuffers[i];
		bindedBuffer.area = AreaCloner::Map(swapChain.buffers[i].ref.area.id);
		bindedBuffer.bitmap.SetTo(new BBitmap(
			bindedBuffer.area->GetArea(),
			buffer.ref.offset,
			BRect(0, 0, buffer.format.width - 1, buffer.format.height - 1),
			0,
			buffer.format.colorSpace,
			buffer.format.bytesPerRow
		));
	}
	return B_OK;
}

status_t SwapChainBindBitmap::Alloc(ObjectDeleter<SwapChain> &swapChain, const SwapChainSpec &spec)
{
	swapChain.SetTo(SwapChain::New(spec.bufferCnt));
	swapChain->presentEffect = spec.presentEffect;

	fBindedBuffers.SetTo(new BindedBuffer[spec.bufferCnt]);
	for (uint32 i = 0; i < spec.bufferCnt; i++) {
		auto &buffer = swapChain->buffers[i];
		auto &mappedBuffer = fBindedBuffers[i];

		mappedBuffer.bitmap.SetTo(new BBitmap(
			BRect(
				0, 0,
				(spec.width != 0 ? spec.width : 1) - 1, // !!!
				(spec.height != 0 ? spec.height : 1) - 1 // !!!
			),
			0, spec.colorSpace
		));
		CheckRet(mappedBuffer.bitmap->InitCheck());
		CheckRet(VideoBufferFromBitmap(buffer, *mappedBuffer.bitmap.Get()));
	}
	return B_OK;
}
