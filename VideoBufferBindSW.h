#pragma once

#include <AutoDeleter.h>
#include <Referenceable.h>
#include "AreaCloner.h"
#include "VideoBuffer.h"

class MappedBuffer;
class MappedArea;
class SwapChain;


class _EXPORT SwapChainBindSW {
public:
	struct BindedBuffer {
		BReference<MappedArea> area;
		AreaDeleter ownedArea;
		uint8* bits;
	};

private:
	ArrayDeleter<BindedBuffer> fBindedBuffers;

public:
	void Unset();
	void ConnectTo(const SwapChain &swapChain);
	void Alloc(ObjectDeleter<SwapChain> &swapChain, const SwapChainSpec &spec);

	BindedBuffer *Buffers() {return fBindedBuffers.Get();}
};
