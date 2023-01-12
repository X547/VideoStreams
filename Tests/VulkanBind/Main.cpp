#include <VulkanDevice.h>
#include <VideoBufferBindVulkan.h>
#include <VideoBufferUtils.h>
#include <stdio.h>
#include <string.h>


#define CheckRet(err) { \
	status_t _err = (err); \
	if (_err < B_OK) { \
		fprintf(stderr, "[!] %s:%d: error %" B_PRIx32 " (%s)\n", __FILE__, __LINE__, _err, strerror(_err)); \
		return _err; \
	} \
} \


void DumpBindedBuffers(SwapChainBindVulkan::BindedBuffer *buffers, uint32 count)
{
	printf("bindedBuffers:\n");
	printf("  buffers: %p\n", buffers);
	for (uint32 i = 0; i < count; i++) {
		const SwapChainBindVulkan::BindedBuffer &buffer = buffers[i];
		printf("  %" B_PRIu32 "\n", i);
		printf("    image: %p\n", buffer.image);
		printf("    memory: %p\n", buffer.memory);
	}
}

status_t TestBind(VulkanDevice &dev)
{
	SwapChainSpec spec {
		.size = sizeof(SwapChainSpec),
		.presentEffect = presentEffectSwap,
		.bufferCnt = 2,
		.kind = bufferRefGpu,
		.width = 640,
		.height = 480,
		.colorSpace = B_RGBA32
	};

	ObjectDeleter<SwapChain> swapChain;
	SwapChainBindVulkan exportBind(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
	exportBind.SetDevice(dev.GetPhysDevice(), dev.GetDevice());
	CheckRet(exportBind.Alloc(swapChain, spec));
	exportBind.Dump();

	DumpSwapChain(*swapChain.Get());

	SwapChainBindVulkan importBind(VK_IMAGE_USAGE_SAMPLED_BIT);
	importBind.SetDevice(dev.GetPhysDevice(), dev.GetDevice());
	CheckRet(importBind.ConnectTo(*swapChain.Get()));
	importBind.Dump();
	return B_OK;
}


int main()
{
	VulkanDevice dev;
	if (dev.Init() != VK_SUCCESS) return 1;
	TestBind(dev);
	printf("[WAIT]"); fgetc(stdin);
	return 0;
}
