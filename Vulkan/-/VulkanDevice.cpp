#include "VulkanDevice.h"
#include <vector>
#include <optional>
#include <set>

#define CheckRet(err) {status_t _err = (err); if (_err < B_OK) return _err;}


struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;

	bool isComplete() {
		return graphicsFamily.has_value();
	}
};

static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}


VulkanDevice::VulkanDevice()
{
}

VulkanDevice::~VulkanDevice()
{
}

status_t VulkanDevice::CreateInstance()
{
	std::vector<const char*> extensions;

	VkApplicationInfo appInfo {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "VideoStreams",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "VideoStreams",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0,
	};

	VkInstanceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
	};

	if (vkCreateInstance(&createInfo, nullptr, &fInstance) != VK_SUCCESS) {
		return B_ERROR;
	}

	return B_OK;
}

status_t VulkanDevice::PickPhysDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(fInstance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		return B_ERROR;
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(fInstance, &deviceCount, devices.data());
	fPhysDev = devices[0];
	return B_OK;
}

status_t VulkanDevice::CreateDevice()
{
	std::vector<const char*> deviceExtensions;

	QueueFamilyIndices indices = FindQueueFamilies(fPhysDev);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value()};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily: uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = queueFamily,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority
		};
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures {
	};

	VkDeviceCreateInfo createInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
		.ppEnabledExtensionNames = deviceExtensions.data(),
		.pEnabledFeatures = &deviceFeatures,
	};

	if (vkCreateDevice(fPhysDev, &createInfo, nullptr, &fDevice) != VK_SUCCESS) {
		return B_ERROR;
	}

	return B_OK;
}

status_t VulkanDevice::Init()
{
	CheckRet(CreateInstance());
	CheckRet(CreateDevice());
	return B_OK;
}
