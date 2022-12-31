#pragma once

#include <vulkan/vulkan.h>
#include <SupportDefs.h>


class VulkanDevice {
private:
	VkInstance fInstance{};
	VkPhysicalDevice fPhysDev{};
	VkDevice fDevice{};

	status_t CreateInstance();
	status_t PickPhysDevice();
	status_t CreateDevice();

public:
	VulkanDevice();
	~VulkanDevice();
	status_t Init();
};
