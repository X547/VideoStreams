#include "GpuProducer.h"
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <xf86drm.h>
#include <amdgpu.h>
#include <amdgpu_drm.h>


static void Check(status_t res)
{
	if (res < B_OK) {
		printf("error %#" B_PRIx32 "(%s)\n", res, strerror(res));
		abort();
	}
}


TestProducer::TestProducer(const char* name):
	VideoProducer(name)
{
	{
		uint32_t majorVersion, minorVersion;
		FileDescriptorCloser devFd(open("/dev/graphics/radeon_gfx_010000", B_READ_WRITE | O_CLOEXEC));
		amdgpu_device_handle dev{};
		Check(amdgpu_device_initialize(devFd.Get(), &majorVersion, &minorVersion, &dev));
		fDev.SetTo(dev);
	}
	printf("dev: %p\n", fDev.Get());
}

TestProducer::~TestProducer()
{
	fBufferCnt = 0;
}

void TestProducer::Connected(bool isActive)
{
	enum {
		bufferCount = 8,
	};
	if (false) {
		SwapChainSpec spec {
			.size = sizeof(SwapChainSpec),
			.presentEffect = presentEffectSwap,
			.bufferCnt = bufferCount,
			.kind = bufferRefGpu,
			.colorSpace = B_RGBA32,
		};
		RequestSwapChain(spec);
	} else {
		uint32 width = 1920;
		uint32 height = 1080;
		uint32 stride = width*4;
		uint32 size = stride*height;

		fBufs.SetTo(new AmdgpuBoRef[bufferCount]);
		VideoBuffer vidBufs[bufferCount] = {};
		for (uint32 i = 0; i < bufferCount; i++) {
			VideoBuffer &vidBuf = vidBufs[i];

			amdgpu_bo_handle bo = NULL;
			struct amdgpu_bo_alloc_request allocReq {
				.alloc_size = size,
				.phys_alignment = 0x1000,
				.preferred_heap = AMDGPU_GEM_DOMAIN_VRAM,
				.flags = true ? AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED : AMDGPU_GEM_CREATE_NO_CPU_ACCESS,
			};
			Check(amdgpu_bo_alloc(fDev.Get(), &allocReq, &bo));
			fBufs[i].SetTo(bo);
			uint32_t boFd = UINT32_MAX;
			Check(amdgpu_bo_export(fBufs[i].Get(), amdgpu_bo_handle_type_dma_buf_fd, &boFd));
			printf("fBufs[%" B_PRIu32 "]: %p\n", i, fBufs[i].Get());

			vidBuf.id = i;
			vidBuf.ref.offset = 0;
			vidBuf.ref.size = size;
			vidBuf.ref.kind = bufferRefGpu;
			vidBuf.ref.gpu.fd = (int)boFd;
			vidBuf.ref.gpu.fenceFd = -1;
			vidBuf.format.bytesPerRow = stride;
			vidBuf.format.width = width;
			vidBuf.format.height = height;
			vidBuf.format.colorSpace = B_RGBA32;
		}
		SwapChain swapChain {
			.size = sizeof(SwapChain),
			.presentEffect = presentEffectSwap,
			.bufferCnt = bufferCount,
			.buffers = vidBufs
		};
		fBufferCnt = bufferCount;
		SetSwapChain(&swapChain);
	}
}

void TestProducer::SwapChainChanged(bool isValid)
{
	VideoProducer::SwapChainChanged(isValid);
	if (!OwnsSwapChain()) {
		fBufs.Unset();
		fBufferCnt = 0;
	}
	if (!isValid)
		return;

	DumpSwapChain(GetSwapChain());

	if (!OwnsSwapChain()) {
		DumpSwapChain(GetSwapChain());
		fBufferCnt = GetSwapChain().bufferCnt;
		fBufs.SetTo(new AmdgpuBoRef[fBufferCnt]);
		for (uint32 i = 0; i < fBufferCnt; i++) {
			amdgpu_bo_import_result importRes{};
			Check(amdgpu_bo_import(fDev.Get(), amdgpu_bo_handle_type_dma_buf_fd, GetSwapChain().buffers[i].ref.gpu.fd, &importRes));
			fBufs[i].SetTo(importRes.buf_handle);
			printf("fBufs[%" B_PRIu32 "]: %p\n", i, fBufs[i].Get());
		}
	}
	Produce();
}

void TestProducer::Produce()
{
	int32 bufId = AllocBuffer();
	if (bufId < 0) {
		printf("[!] AllocBuffer() failed\n");
		return;
	}

	amdgpu_bo_handle buf = fBufs[bufId].Get();

	uint32 *bits;
	Check(amdgpu_bo_cpu_map(buf, (void**)&bits));
	const auto &bufDesc = GetSwapChain().buffers[bufId];
	for (int32 y = 0; y < bufDesc.format.height; y++) {
		uint32 *line = (uint32*)((uint8*)bits + y*bufDesc.format.bytesPerRow);
#if 0
		for (int32 x = 0; x < bufDesc.format.width; x++) {
			line[x] = x + Era();
		}
#endif
		memset(line, y + Era(), bufDesc.format.width*4);
	}
	Check(amdgpu_bo_cpu_unmap(buf));

	Present(bufId);
}

void TestProducer::Presented(const PresentedInfo &presentedInfo)
{
	Produce();
}

void TestProducer::MessageReceived(BMessage* msg)
{
	VideoProducer::MessageReceived(msg);
};
