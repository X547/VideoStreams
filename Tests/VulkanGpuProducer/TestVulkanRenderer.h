#pragma once

#include <VulkanDevice.h>
#include <VideoBufferBindVulkan.h>

#include <AutoDeleter.h>


class TestVulkanRenderer {
private:
	struct RenderImage {
		VkImageCreateInfo *imageInfo = NULL;
		VkImage image = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		VkFramebuffer framebuffer = VK_NULL_HANDLE;
	};

	VulkanDevice &fDev;

	VkRenderPass fRenderPass = VK_NULL_HANDLE;
	VkCommandPool fCommandPool = VK_NULL_HANDLE;

	SwapChainBindVulkan *fBind;
	ArrayDeleter<RenderImage> fImages;

private:
	VkResult SubmitWork(VkCommandBuffer cmdBuffer);

	VkResult InitRenderPass();

	VkResult DrawRenderPass(VkCommandBuffer cmdBuf, int32 bufferId);
	VkResult DrawCmds(VkCommandBuffer cmdBuf, int32 bufferId);

public:
	TestVulkanRenderer(VulkanDevice &dev);
	~TestVulkanRenderer();

	VkResult Init();
	VkResult ConnectTo(SwapChainBindVulkan *bind);

	VkResult Draw(int32 bufferId);
};
