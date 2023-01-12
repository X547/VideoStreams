#include "ConsumerGpuView.h"
#include "VideoBufferUtils.h"
#include <VideoBufferBindVulkan.h>
#include <VulkanDevice.h>

#include <stdio.h>
#include <Bitmap.h>
#include <Region.h>
#include <Looper.h>
#include <AutoLocker.h>
#include <AutoDeleterOS.h>

#define CheckRet(err) {status_t _err = (err); if (_err < B_OK) return _err;}

#define VkCheck(err) { \
	VkResult _err = (err); \
	if (_err != VK_SUCCESS) { \
		fprintf(stderr, "[!] %s:%d: Vulkan error %d\n", __FILE__, __LINE__, _err); \
		abort(); \
	} \
} \

#define VkCheckRet(err) { \
	VkResult _err = (err); \
	if (_err != VK_SUCCESS) { \
		fprintf(stderr, "[!] %s:%d: Vulkan error %d\n", __FILE__, __LINE__, _err); \
		return _err; \
	} \
} \


class ViewGpuConsumer final: public VideoConsumer
{
private:
	friend class ConsumerGpuView;

	BView* fView;
	VulkanDevice fVkDev;
	SwapChainBindVulkan fSwapChainBind;

	VkCommandPool fCommandPool = VK_NULL_HANDLE;
	struct {
		VkImage image;
		VkDeviceMemory memory;
		AreaDeleter area;
		ObjectDeleter<BBitmap> bitmap;
	} fBuffer;

private:
	void KeepOldDisplayBitmap();

	VkResult SubmitWork(VkCommandBuffer cmdBuffer);
	VkResult CreateBuffer(const VkImageCreateInfo &srcCreateInfo);
	VkResult CopyToBuffer(VkImage srcImage, int32_t width, int32_t height);

public:
	ViewGpuConsumer(const char* name, BView* view);
	virtual ~ViewGpuConsumer();

	void Connected(bool isActive) final;
	status_t SwapChainRequested(const SwapChainSpec& spec) final;
	void SwapChainChanged(bool isValid) final;
	void Present(int32 bufferId, const BRegion* dirty) final;
};


ViewGpuConsumer::ViewGpuConsumer(const char* name, BView* view):
	VideoConsumer(name),
	fView(view),
	fSwapChainBind(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
{
	if (fVkDev.Init() < B_OK) abort();
	fSwapChainBind.SetDevice(fVkDev.GetPhysDevice(), fVkDev.GetDevice());

	VkCommandPoolCreateInfo cmdPoolInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = 0
	};
	VkCheck(vkCreateCommandPool(fVkDev.GetDevice(), &cmdPoolInfo, nullptr, &fCommandPool));
}

ViewGpuConsumer::~ViewGpuConsumer()
{
	printf("-ViewGpuConsumer: "); WriteMessenger(BMessenger(this)); printf("\n");
}


// #pragma mark - Vulkan stuff

static void InsertImageMemoryBarrier(
	VkCommandBuffer cmdbuffer,
	VkImage image,
	VkAccessFlags srcAccessMask,
	VkAccessFlags dstAccessMask,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout,
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask,
	VkImageSubresourceRange subresourceRange
) {
	VkImageMemoryBarrier imageMemoryBarrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = srcAccessMask,
		.dstAccessMask = dstAccessMask,
		.oldLayout = oldImageLayout,
		.newLayout = newImageLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = subresourceRange
	};

	vkCmdPipelineBarrier(
		cmdbuffer,
		srcStageMask,
		dstStageMask,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier
	);
}

VkResult ViewGpuConsumer::SubmitWork(VkCommandBuffer cmdBuffer)
{
	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmdBuffer
	};
	VkFence fence;
	VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
	VkCheckRet(vkCreateFence(fVkDev.GetDevice(), &fenceInfo, nullptr, &fence));
	VkCheckRet(vkQueueSubmit(fVkDev.GetQueue(), 1, &submitInfo, fence));
	VkCheckRet(vkWaitForFences(fVkDev.GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX));
	vkDestroyFence(fVkDev.GetDevice(), fence, nullptr);
	return VK_SUCCESS;
}

VkResult ViewGpuConsumer::CreateBuffer(const VkImageCreateInfo &srcCreateInfo)
{
	VkImageCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_B8G8R8A8_UNORM,
		.extent = {
			.width = srcCreateInfo.extent.width,
			.height = srcCreateInfo.extent.height,
			.depth = 1
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_LINEAR,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VkCheckRet(vkCreateImage(fVkDev.GetDevice(), &createInfo, NULL, &fBuffer.image));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(fVkDev.GetDevice(), fBuffer.image, &memRequirements);
	size_t memTypeIdx = fVkDev.GetMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void *memAreaAdr = NULL;
	fBuffer.area.SetTo(create_area("buffer", &memAreaAdr, B_ANY_ADDRESS, memRequirements.size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA | B_CLONEABLE_AREA));
	if (!fBuffer.area.IsSet())
		return VK_ERROR_OUT_OF_HOST_MEMORY;

	VkMemoryAllocateInfo memAllocInfo {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = (uint32_t)memTypeIdx
	};

	VkImportMemoryHostPointerInfoEXT hostPtrInfo {
		.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT,
		.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT,
		.pHostPointer = memAreaAdr
	};
	memAllocInfo.pNext = &hostPtrInfo;

	VkCheckRet(vkAllocateMemory(fVkDev.GetDevice(), &memAllocInfo, nullptr, &fBuffer.memory));
	VkCheckRet(vkBindImageMemory(fVkDev.GetDevice(), fBuffer.image, fBuffer.memory, 0));

	VkImageSubresource subResource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT};
	VkSubresourceLayout subResourceLayout;
	vkGetImageSubresourceLayout(fVkDev.GetDevice(), fBuffer.image, &subResource, &subResourceLayout);
	fBuffer.bitmap.SetTo(new(std::nothrow) BBitmap(fBuffer.area.Get(), 0, BRect(0, 0, createInfo.extent.width - 1, createInfo.extent.height - 1), B_BITMAP_IS_AREA, B_RGB32, subResourceLayout.rowPitch));
	if (!fBuffer.bitmap.IsSet())
		return VK_ERROR_OUT_OF_HOST_MEMORY;

	return VK_SUCCESS;
}

VkResult ViewGpuConsumer::CopyToBuffer(VkImage srcImage, int32_t width, int32_t height)
{
	// Do the actual blit from the offscreen image to our host visible destination image
	VkCommandBufferAllocateInfo cmdBufAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = fCommandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};
	VkCommandBuffer copyCmd;
	VkCheckRet(vkAllocateCommandBuffers(fVkDev.GetDevice(), &cmdBufAllocateInfo, &copyCmd));
	VkCommandBufferBeginInfo cmdBufInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	VkCheckRet(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));

	// Transition destination image to transfer destination layout
	InsertImageMemoryBarrier(
		copyCmd,
		fBuffer.image,
		0,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	);

	VkOffset3D blitSize{.x = width, .y = height, .z = 1};
	VkImageBlit imageBlitRegion{
		.srcSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1
		},
		.srcOffsets = {{}, blitSize},
		.dstSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1,
		},
		.dstOffsets = {{}, blitSize}
	};

	// Issue the blit command
	vkCmdBlitImage(
		copyCmd,
		srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		fBuffer.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&imageBlitRegion,
		VK_FILTER_NEAREST
	);

	// Transition destination image to general layout, which is the required layout for mapping the image memory later on
	InsertImageMemoryBarrier(
		copyCmd,
		fBuffer.image,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_MEMORY_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	);

	VkCheckRet(vkEndCommandBuffer(copyCmd));

	VkCheckRet(SubmitWork(copyCmd));
	vkFreeCommandBuffers(fVkDev.GetDevice(), fCommandPool, 1, &copyCmd);

	return VK_SUCCESS;
}


void ViewGpuConsumer::Connected(bool isActive)
{
	if (isActive) {
		printf("ViewGpuConsumer: connected to ");
		WriteMessenger(Link());
		printf("\n");
	} else {
		printf("ViewGpuConsumer: disconnected\n");
		SetSwapChain(NULL);
		fSwapChainBind.Unset();
		fView->Invalidate();
	}
}


// #pragma mark -


void ViewGpuConsumer::KeepOldDisplayBitmap()
{
}

status_t ViewGpuConsumer::SwapChainRequested(const SwapChainSpec& spec)
{
	printf("ViewGpuConsumer::SwapChainRequested(%" B_PRIuSIZE ")\n", spec.bufferCnt);

	KeepOldDisplayBitmap();

	ObjectDeleter<SwapChain> swapChain;
	CheckRet(fSwapChainBind.Alloc(swapChain, spec));
	SetSwapChain(swapChain.Get());

	return B_OK;
}

void ViewGpuConsumer::SwapChainChanged(bool isValid)
{
	printf("ViewGpuConsumer::SwapChainChanged(%d)\n", isValid);
	VideoConsumer::SwapChainChanged(isValid);
	if (!isValid) {
		KeepOldDisplayBitmap();
		fSwapChainBind.Unset();
		return;
	}
	if (!OwnsSwapChain()) {
		KeepOldDisplayBitmap();
		fSwapChainBind.ConnectTo(GetSwapChain());
	}
	DumpSwapChain(GetSwapChain());
	fSwapChainBind.Dump();
	VkCheck(CreateBuffer(fSwapChainBind.BufferAt(0).imageInfo));
}


void ViewGpuConsumer::Present(int32 bufferId, const BRegion* dirty)
{
	(void)dirty;
	if (fBuffer.bitmap.IsSet()) {
		VkCheck(CopyToBuffer(fSwapChainBind.BufferAt(bufferId).image, (int32)fBuffer.bitmap->Bounds().Width() + 1, (int32)fBuffer.bitmap->Bounds().Height() + 1));
		fView->Invalidate();
	}
	PresentedInfo presentedInfo{};
	Presented(presentedInfo);
}


void ConsumerGpuView::AttachedToWindow()
{
	fConsumer.SetTo(new ViewGpuConsumer("testConsumer", this));
	Looper()->AddHandler(fConsumer.Get());
	printf("+ViewGpuConsumer: "); WriteMessenger(BMessenger(fConsumer.Get())); printf("\n");
}

void ConsumerGpuView::DetachedFromWindow()
{
	fConsumer.Unset();
}

void ConsumerGpuView::FrameResized(float newWidth, float newHeight)
{
	(void)newWidth;
	(void)newHeight;
}

void ConsumerGpuView::Draw(BRect dirty)
{
	(void)dirty;
	if (fConsumer->fBuffer.bitmap.IsSet()) {
		DrawBitmap(fConsumer->fBuffer.bitmap.Get(), B_ORIGIN);
	}
}


VideoConsumer* ConsumerGpuView::GetConsumer()
{
	return fConsumer.Get();
}
