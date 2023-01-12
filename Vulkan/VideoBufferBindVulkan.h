#pragma once

#include "VideoBufferBind.h"
#include <Referenceable.h>

#include <vulkan/vulkan.h>

class SwapChain;
class SwapChainSpec;


class _EXPORT SwapChainBindVulkan: public SwapChainBind {
public:
	struct BindedBuffer {
		VkImageCreateInfo imageInfo {};
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
	};

private:
	VkPhysicalDevice fPhysDev = VK_NULL_HANDLE;
	VkDevice fDevice = VK_NULL_HANDLE;
	VkImageUsageFlags fImageUsage;

	struct {
		PFN_vkGetMemoryFdKHR GetMemoryFdKHR;
		PFN_vkGetMemoryFdPropertiesKHR GetMemoryFdPropertiesKHR;
	} fFns {};

	uint32 fBufferCnt = 0;
	ArrayDeleter<BindedBuffer> fBindedBuffers;

public:
	SwapChainBindVulkan(VkImageUsageFlags imageUsage);
	~SwapChainBindVulkan();
	void SetDevice(VkPhysicalDevice physDev, VkDevice dev);
	void Unset() override;
	status_t ConnectTo(const SwapChain &swapChain) override;
	status_t Alloc(ObjectDeleter<SwapChain> &swapChain, const SwapChainSpec &spec) override;

	uint32 BufferCount() {return fBufferCnt;}
	BindedBuffer &BufferAt(size_t idx);

	void Dump();
};
