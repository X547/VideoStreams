#include "VideoBufferBindBitmap.h"
#include "VideoBuffer.h"


void SwapChainBindBitmap::ConnectTo(const SwapChain *swapChain)
{
	if (swapChain == NULL) {
		fBindedBuffers.Unset();
		return;
	}
	fBindedBuffers.SetTo(new BindedBuffer[swapChain->bufferCnt]);
	for (uint32 i = 0; i < swapChain->bufferCnt; i++) {
		const auto &buffer = swapChain->buffers[i];
		auto &bindedBuffer = fBindedBuffers[i];
		bindedBuffer.area = AreaCloner::Map(swapChain->buffers[i].ref.area.id);
		bindedBuffer.bitmap.SetTo(new BBitmap(
			bindedBuffer.area->GetArea(),
			buffer.ref.offset,
			BRect(0, 0, buffer.format.width - 1, buffer.format.height - 1),
			0,
			buffer.format.colorSpace,
			buffer.format.bytesPerRow
		));
	}
}
