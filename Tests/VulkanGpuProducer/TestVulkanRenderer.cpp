#include "TestVulkanRenderer.h"

#include <stdio.h>
#include <math.h>

#define VkCheckRet(err) { \
	VkResult _err = (err); \
	if (_err != VK_SUCCESS) { \
		fprintf(stderr, "[!] %s:%d: Vulkan error %d\n", __FILE__, __LINE__, _err); \
		return _err; \
	} \
} \


// TODO: fix resource leaks


TestVulkanRenderer::TestVulkanRenderer(VulkanDevice &dev): fDev(dev)
{
}

TestVulkanRenderer::~TestVulkanRenderer()
{
}


VkResult TestVulkanRenderer::SubmitWork(VkCommandBuffer cmdBuffer)
{
	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmdBuffer
	};
	VkFence fence;
	VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
	VkCheckRet(vkCreateFence(fDev.GetDevice(), &fenceInfo, nullptr, &fence));
	VkCheckRet(vkQueueSubmit(fDev.GetQueue(), 1, &submitInfo, fence));
	VkCheckRet(vkWaitForFences(fDev.GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX));
	vkDestroyFence(fDev.GetDevice(), fence, nullptr);
	return VK_SUCCESS;
}

VkResult TestVulkanRenderer::InitRenderPass()
{
	VkAttachmentDescription attachments[] {
		{
			.format = VK_FORMAT_B8G8R8A8_UNORM, // !!!
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		}
	};

	VkAttachmentReference colorReference = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpassDescription = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorReference,
	};

	VkSubpassDependency dependencies[] {
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		}
	};

	VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = B_COUNT_OF(attachments),
		.pAttachments = attachments,
		.subpassCount = 1,
		.pSubpasses = &subpassDescription,
		.dependencyCount = B_COUNT_OF(dependencies),
		.pDependencies = dependencies
	};

	VkCheckRet(vkCreateRenderPass(fDev.GetDevice(), &renderPassInfo, nullptr, &fRenderPass));

	return VK_SUCCESS;
}

VkResult TestVulkanRenderer::Init()
{
	VkCommandPoolCreateInfo cmdPoolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = 0
	};
	VkCheckRet(vkCreateCommandPool(fDev.GetDevice(), &cmdPoolInfo, nullptr, &fCommandPool));

	VkCheckRet(InitRenderPass());

	return VK_SUCCESS;
}

VkResult TestVulkanRenderer::ConnectTo(SwapChainBindVulkan *bind)
{
	fImages.Unset();
	fBind = bind;
	if (bind == NULL) return VK_SUCCESS;
	fImages.SetTo(new RenderImage[bind->BufferCount()]);
	for (uint32 i = 0; i < bind->BufferCount(); i++) {
		SwapChainBindVulkan::BindedBuffer &bindedBuffer = bind->BufferAt(i);
		RenderImage &renderImage = fImages[i];

		renderImage.imageInfo = &bindedBuffer.imageInfo;

		VkImageViewCreateInfo colorAttachmentView = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = bindedBuffer.image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = bindedBuffer.imageInfo.format,
			.components = {
				VK_COMPONENT_SWIZZLE_R,
				VK_COMPONENT_SWIZZLE_G,
				VK_COMPONENT_SWIZZLE_B,
				VK_COMPONENT_SWIZZLE_A
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1,
			}
		};
		VkCheckRet(vkCreateImageView(fDev.GetDevice(), &colorAttachmentView, nullptr, &renderImage.view));

		VkImageView attachments[] = {renderImage.view};
		VkFramebufferCreateInfo frameBufferCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = fRenderPass,
			.attachmentCount = B_COUNT_OF(attachments),
			.pAttachments = attachments,
			.width = bindedBuffer.imageInfo.extent.width,
			.height = bindedBuffer.imageInfo.extent.height,
			.layers = 1
		};
		VkCheckRet(vkCreateFramebuffer(fDev.GetDevice(), &frameBufferCreateInfo, nullptr, &renderImage.framebuffer));
	}
	return VK_SUCCESS;
}

VkResult TestVulkanRenderer::Draw(int32 bufferId)
{
	VkCommandBufferAllocateInfo cmdBufAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = fCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	VkCommandBuffer cmdBuf;
	VkCheckRet(vkAllocateCommandBuffers(fDev.GetDevice(), &cmdBufAllocateInfo, &cmdBuf));

	VkCommandBufferBeginInfo cmdBufInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	VkCheckRet(vkBeginCommandBuffer(cmdBuf, &cmdBufInfo));
	VkCheckRet(DrawRenderPass(cmdBuf, bufferId));
	VkCheckRet(vkEndCommandBuffer(cmdBuf));

	VkCheckRet(SubmitWork(cmdBuf));

	vkFreeCommandBuffers(fDev.GetDevice(), fCommandPool, 1, &cmdBuf);

	return VK_SUCCESS;
}

VkResult TestVulkanRenderer::DrawRenderPass(VkCommandBuffer cmdBuf, int32 bufferId)
{
	VkClearValue clearValues[] = {
		{.color = {0.0f, 0.2f, 0.4f, 1.0f}}
	};
	VkRenderPassBeginInfo renderPassBeginInfo {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = fRenderPass,
		.framebuffer = fImages[bufferId].framebuffer,
		.renderArea = {
			.offset = {.x = 0, .y = 0},
			.extent = {
				.width = fImages[bufferId].imageInfo->extent.width,
				.height = fImages[bufferId].imageInfo->extent.height,
			}
		},
		.clearValueCount = B_COUNT_OF(clearValues),
		.pClearValues = clearValues
	};
	vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkCheckRet(DrawCmds(cmdBuf, bufferId));

	vkCmdEndRenderPass(cmdBuf);

	return VK_SUCCESS;
}

void static FillRect(VkCommandBuffer cmdBuf, const VkRect2D &rect, const VkClearColorValue &color)
{
	VkClearAttachment attachments[] {{
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.clearValue = {.color = color}
	}};
	VkClearRect clearRects[] {{
		.rect = rect,
		.layerCount = 1
	}};
	vkCmdClearAttachments(cmdBuf, B_COUNT_OF(attachments), attachments, B_COUNT_OF(clearRects), clearRects);
}

VkResult TestVulkanRenderer::DrawCmds(VkCommandBuffer cmdBuf, int32 bufferId)
{
	(void)bufferId;

	static bigtime_t prevTime = system_time();
	bigtime_t time = system_time();
	float dt = (time - prevTime) / 1000000.0f;
	prevTime = time;
	static float phase = 0;
	phase += dt;
	if (phase >= 1.0f) phase -= 1.0f;

	for (uint32_t i = 0; i < 48; i++) {
		VkRect2D rect{.offset = {32 + 16*cosf(2*M_PI*phase) + 8*i, 32 + 16*sinf(2*M_PI*phase) + 8*i}, .extent = {64, 64}};
		VkClearColorValue colors[] {
			{1.0f, 0.0f, 0.0f, 1.0f},
			{0.0f, 1.0f, 0.0f, 1.0f},
			{0.0f, 0.0f, 1.0f, 1.0f}
		};
		FillRect(cmdBuf, rect, colors[i % 3]);
	}

	return VK_SUCCESS;
}
