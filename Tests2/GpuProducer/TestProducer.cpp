#include "TestProducer.h"
#include <stdio.h>

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

static void ImportBuffer(amdgpu_device_handle dev, amdgpu_bo_handle &bo, uint32_t srcBoHandle)
{
	uint32_t dstHandle = 0;
	int bufFd = -1;
	amdgpu_bo_import_result importRes{};
	Check(drmPrimeHandleToFD(amdgpu_device_get_fd(dev), srcBoHandle, 0, &bufFd));
	Check(amdgpu_bo_import(dev, amdgpu_bo_handle_type_dma_buf_fd, bufFd, &importRes));
	bo = importRes.buf_handle;
	close(bufFd);
}

#if 0
static void ImportExternalBuffer(amdgpu_device_handle dev, amdgpu_bo_handle &bo, int32_t srcPid, uint32_t srcBoHandle)
{
	uint32_t dstHandle = 0;
	int bufFd = -1;
	amdgpu_bo_import_result importRes{};
	Check(radeonGfxDuplicateHandle(amdgpu_device_get_fd(dev), &dstHandle, srcBoHandle, radeonHandleBuffer, B_CURRENT_TEAM, srcPid));
	Check(drmPrimeHandleToFD(amdgpu_device_get_fd(dev), dstHandle, 0, &bufFd));
	Check(amdgpu_bo_import(dev, amdgpu_bo_handle_type_dma_buf_fd, bufFd, &importRes));
	bo = importRes.buf_handle;
	close(bufFd);
}
#endif


TestProducer::TestProducer(const char* name):
	VideoProducer(name)
{
	{
		uint32_t majorVersion, minorVersion;
		int devFd = open("/dev/null", O_RDWR | O_CLOEXEC);
		Check(amdgpu_device_initialize(devFd, &majorVersion, &minorVersion, &fDev));
		close(devFd); devFd = -1;
	}
	printf("dev: %p\n", fDev);
}

TestProducer::~TestProducer()
{
	for (uint32 i = 0; i < fBufferCnt; i++) {
		amdgpu_bo_free(fBufs[i]); fBufs[i] = NULL;
	}
	fBufferCnt = 0;
	amdgpu_device_deinitialize(fDev); fDev = NULL;
}

void TestProducer::Connected(bool isActive)
{
	if (false) {
		SwapChainSpec spec {
			.size = sizeof(SwapChainSpec),
			.presentEffect = presentEffectSwap,
			.bufferCnt = 2,
			.kind = bufferRefGpu,
			.colorSpace = B_RGBA32,
		};
		RequestSwapChain(spec);
	} else {
		enum {
			bufferCount = 8,
		};
		uint32 width = 1920;
		uint32 height = 1080;
		uint32 stride = width*4;
		uint32 size = stride*height;

		fBufs.SetTo(new amdgpu_bo_handle[bufferCount]);
		VideoBuffer vidBufs[bufferCount] = {};
		for (uint32 i = 0; i < bufferCount; i++) {
			struct amdgpu_bo_alloc_request allocReq {
				.alloc_size = size,
				.phys_alignment = 0x1000,
				.preferred_heap = AMDGPU_GEM_DOMAIN_VRAM,
				.flags = true ? AMDGPU_GEM_CREATE_CPU_ACCESS_REQUIRED : AMDGPU_GEM_CREATE_NO_CPU_ACCESS,
			};
			uint32_t boHandle = UINT32_MAX;
			amdgpu_bo_handle bo = NULL;
			Check(amdgpu_bo_alloc(fDev, &allocReq, &bo));
			fBufs[i] = bo;
			Check(amdgpu_bo_export(fBufs[i], amdgpu_bo_handle_type_kms, &boHandle));
			printf("fBufs[%" B_PRIu32 "]: %p\n", i, fBufs[i]);

			vidBufs[i].id = i,
			vidBufs[i].ref.offset = 0,
			vidBufs[i].ref.size = size,
			vidBufs[i].ref.kind = bufferRefGpu,
			vidBufs[i].ref.gpu.id = boHandle,
			vidBufs[i].ref.gpu.team = getpid(),
			vidBufs[i].format.bytesPerRow = stride,
			vidBufs[i].format.width = width,
			vidBufs[i].format.height = height,
			vidBufs[i].format.colorSpace = B_RGBA32;
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
		for (uint32 i = 0; i < fBufferCnt; i++) {
			amdgpu_bo_free(fBufs[i]); fBufs[i] = NULL;
		}
		fBufferCnt = 0;
	}
	if (!isValid)
		return;

	DumpSwapChain(GetSwapChain());

	if (!OwnsSwapChain()) {
		DumpSwapChain(GetSwapChain());
		fBufferCnt = GetSwapChain().bufferCnt;
		fBufs.SetTo(new amdgpu_bo_handle[fBufferCnt]);
		for (uint32 i = 0; i < fBufferCnt; i++) {
			ImportBuffer(fDev, fBufs[i], GetSwapChain().buffers[i].ref.gpu.id);
			printf("fBufs[%" B_PRIu32 "]: %p\n", i, fBufs[i]);
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

	amdgpu_bo_handle buf = fBufs[bufId];
	
#if 1
	uint32 *bits;
	Check(amdgpu_bo_cpu_map(buf, (void**)&bits));
	const auto &bufDesc = GetSwapChain().buffers[bufId];
	for (int32 y = 0; y < bufDesc.format.height; y++) {
		memset((uint8*)bits + y*bufDesc.format.bytesPerRow, y + Era(), bufDesc.format.width*4);
	}
	Check(amdgpu_bo_cpu_unmap(buf));
#endif

	Present(bufId);
}

void TestProducer::Presented()
{
	Produce();
}

void TestProducer::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case stepMsg: {
		return;
	}
	}
	VideoProducer::MessageReceived(msg);
};
