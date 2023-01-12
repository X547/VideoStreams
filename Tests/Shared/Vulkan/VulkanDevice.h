#pragma once

#include <vulkan/vulkan.h>


class VulkanDevice {
private:
	VkInstance fInstance = VK_NULL_HANDLE;
	VkPhysicalDevice fPhysDevice = VK_NULL_HANDLE;
	VkDevice fDevice = VK_NULL_HANDLE;

	VkPhysicalDeviceProperties fProperties{};
	VkPhysicalDeviceFeatures fFeatures{};
	VkPhysicalDeviceMemoryProperties fMemoryProperties{};

	VkQueue fQueue = VK_NULL_HANDLE;

	VkResult CreateInstance();
	VkResult SelectPhysDevice();
	VkResult CreateDevice();

public:
	VkResult Init();
	~VulkanDevice();

	inline VkInstance GetInstance() {return fInstance;}
	inline VkPhysicalDevice GetPhysDevice() {return fPhysDevice;}
	inline VkDevice GetDevice() {return fDevice;}

	inline VkQueue GetQueue() {return fQueue;}

	uint32_t GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);
};
