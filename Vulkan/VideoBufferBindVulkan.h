#pragma once

#include <AutoDeleter.h>
#include <Referenceable.h>

#include <vulkan/vulkan.h>

class SwapChain;
class SwapChainSpec;


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
	void Unset();
	status_t ConnectTo(const SwapChain &swapChain);
	status_t Alloc(ObjectDeleter<SwapChain> &swapChain, const SwapChainSpec &spec);

	BindedBuffer *Buffers() {return fBindedBuffers.Get();}
};
