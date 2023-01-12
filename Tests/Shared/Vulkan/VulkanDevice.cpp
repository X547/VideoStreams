#include "VulkanDevice.h"

#include <stdio.h>
#include <SupportDefs.h>
#include <AutoDeleter.h>

#define VkCheckRet(err) { \
	VkResult _err = (err); \
	if (_err != VK_SUCCESS) { \
		fprintf(stderr, "[!] %s:%d: Vulkan error %d\n", __FILE__, __LINE__, _err); \
		return _err; \
	} \
} \


VkResult VulkanDevice::Init()
{
	VkCheckRet(CreateInstance());
	VkCheckRet(SelectPhysDevice());
	VkCheckRet(CreateDevice());
	return VK_SUCCESS;
}

VulkanDevice::~VulkanDevice()
{
	if (fDevice != VK_NULL_HANDLE)
		vkDestroyDevice(fDevice, nullptr);
	if (fInstance != VK_NULL_HANDLE)
		vkDestroyInstance(fInstance, nullptr);
}

VkResult VulkanDevice::CreateInstance()
{
	const char *instanceLayers[] {NULL};
	const char *instanceExtensions[] {VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME, NULL};

	// create instance
	VkApplicationInfo applicationInfo {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Minimal Vulkan App",
		.applicationVersion = VK_MAKE_VERSION(0, 0, 1),
		.pEngineName = "Minimal",
		.engineVersion = VK_MAKE_VERSION(0, 0, 1),
		.apiVersion = VK_MAKE_VERSION(1, 2, 0)
	};

	VkInstanceCreateInfo instanceCreateInfo {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &applicationInfo,
		.enabledLayerCount = B_COUNT_OF(instanceLayers) - 1,
		.ppEnabledLayerNames = instanceLayers,
		.enabledExtensionCount = B_COUNT_OF(instanceExtensions) - 1,
		.ppEnabledExtensionNames = instanceExtensions
	};

	VkCheckRet(vkCreateInstance(&instanceCreateInfo, nullptr, &fInstance));
	return VK_SUCCESS;
}

VkResult VulkanDevice::SelectPhysDevice()
{
	uint32_t gpuCnt;
	VkCheckRet(vkEnumeratePhysicalDevices(fInstance, &gpuCnt, nullptr));
	ArrayDeleter<VkPhysicalDevice> gpus(new VkPhysicalDevice[gpuCnt]);
	VkCheckRet(vkEnumeratePhysicalDevices(fInstance, &gpuCnt, &gpus[0]));

	fPhysDevice = gpus[0];
	vkGetPhysicalDeviceProperties(fPhysDevice, &fProperties);
	vkGetPhysicalDeviceFeatures(fPhysDevice, &fFeatures);
	vkGetPhysicalDeviceMemoryProperties(fPhysDevice, &fMemoryProperties);

	printf("Selected GPU: %s\n", fProperties.deviceName);

	return VK_SUCCESS;
}

VkResult VulkanDevice::CreateDevice()
{
	const char *deviceExtensions[] {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
		VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME
	};

	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(fPhysDevice, &queueFamilyCount, nullptr);
	ArrayDeleter<VkQueueFamilyProperties> familyProperties(new VkQueueFamilyProperties[queueFamilyCount]);
	vkGetPhysicalDeviceQueueFamilyProperties(fPhysDevice, &queueFamilyCount, &familyProperties[0]);

	uint32_t queueFamilyIndex = 0;
	while (queueFamilyIndex < queueFamilyCount && !(familyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {queueFamilyIndex++;}
	if (!(queueFamilyIndex < queueFamilyCount)) {
		return VK_ERROR_UNKNOWN;
	}

	const float priorities[] {1.0f};
	VkDeviceQueueCreateInfo queueCreateInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = queueFamilyIndex,
		.queueCount = 1,
		.pQueuePriorities = priorities
	};

	VkDeviceCreateInfo deviceCreateInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledExtensionCount = B_COUNT_OF(deviceExtensions),
		.ppEnabledExtensionNames = deviceExtensions
	};
	vkCreateDevice(fPhysDevice, &deviceCreateInfo, nullptr, &fDevice);

	vkGetDeviceQueue(fDevice, queueFamilyIndex, 0, &fQueue);

	return VK_SUCCESS;
}

uint32_t VulkanDevice::GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties)
{
	for (uint32_t i = 0; i < fMemoryProperties.memoryTypeCount; ++i) {
		if ((typeBits & (1 << i)) && (fMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	return 0;
}
