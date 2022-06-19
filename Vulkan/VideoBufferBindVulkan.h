#pragma once

#include <AutoDeleter.h>
#include <Referenceable.h>

#include <vulkan/vulkan.h>

class SwapChain;


class _EXPORT SwapChainBindVulkan {
public:
	struct BindedBuffer {
		VkImage image;
		VkDeviceMemory memory;
	};

private:
	VkPhysicalDevice fPhysDev;
	VkDevice fDevice;

	ArrayDeleter<BindedBuffer> fBindedBuffers;

public:
	void ConnectTo(const SwapChain *swapChain);

	BindedBuffer *Buffers() {return fBindedBuffers.Get();}
};
