#ifndef _VIDEOBUFFER_H_
#define _VIDEOBUFFER_H_


#include <OS.h>
#include <InterfaceDefs.h>


enum PresentEffect {
	presentEffectNone,
	presentEffectSwap,
	presentEffectCopy,
};


struct VideoBuffer
{
	int32 id; // index in SwapChain.buffers
	area_id area;
	size_t offset;
	int32 length;
	int32 bytesPerRow;
	int32 width, height;
	color_space colorSpace;
};


struct BufferSpec {
	color_space colorSpace;
};


struct SwapChainSpec {
	size_t size;
	PresentEffect presentEffect;
	size_t bufferCnt;
	BufferSpec* bufferSpecs;
};


struct SwapChain {
	size_t size;
	uint32 bufferCnt;
	VideoBuffer* buffers;
};


#endif	// _VIDEOBUFFER_H_
