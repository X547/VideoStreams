#pragma once

#include <AutoDeleter.h>
#include <Referenceable.h>
#include <Bitmap.h>
#include "AreaCloner.h"

class MappedArea;
class SwapChain;


class _EXPORT SwapChainBindBitmap {
public:
	struct BindedBuffer {
		BReference<MappedArea> area;
		ObjectDeleter<BBitmap> bitmap;
	};

private:
	ArrayDeleter<BindedBuffer> fBindedBuffers;

public:
	void ConnectTo(const SwapChain *swapChain);

	BindedBuffer *Buffers() {return fBindedBuffers.Get();}
};
