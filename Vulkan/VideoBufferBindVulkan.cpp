#include "VideoBufferBindVulkan.h"
#include "VideoBuffer.h"

#include <xf86drm.h>

#define VkCheckRet(err) {VkResult _err = (err); if (_err != VK_SUCCESS) abort();}


static uint32_t getMemoryTypeIndex(VkPhysicalDevice physDev, uint32_t typeBits, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physDev, &deviceMemoryProperties);
	for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
		if ((typeBits & 1) == 1) {
			if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		typeBits >>= 1;
	}
	return 0;
}


void SwapChainBindVulkan::ConnectTo(const SwapChain *swapChain)
{
	if (swapChain == NULL) {
		fBindedBuffers.Unset();
		return;
	}
	fBindedBuffers.SetTo(new BindedBuffer[swapChain->bufferCnt]);
	for (uint32 i = 0; i < swapChain->bufferCnt; i++) {
		const auto &buffer = swapChain->buffers[i];
		auto &bindedBuffer = fBindedBuffers[i];

		VkImageCreateInfo createInfo {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_B8G8R8A8_UNORM, // !!!
			.extent = {
				buffer.format.width,
				buffer.format.height,
				1
			},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_LINEAR,
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};
		
		switch (buffer.ref.kind) {
			case bufferRefArea: {
				VkCheckRet(vkCreateImage(fDevice, &createInfo, NULL, &bindedBuffer.image));

				VkMemoryRequirements memRequirements;
				vkGetImageMemoryRequirements(fDevice, bindedBuffer.image, &memRequirements);
				size_t memTypeIdx = getMemoryTypeIndex(fPhysDev, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

				VkMemoryDedicatedAllocateInfo dedicateInfo{
					.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
					.image = bindedBuffer.image,
				};
			
				VkMemoryAllocateInfo memAllocInfo{
					.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
					.pNext = &dedicateInfo,
					.allocationSize = memRequirements.size,
					.memoryTypeIndex = (uint32_t)memTypeIdx
				};

				void *memAreaAdr = NULL;
				VkImportMemoryHostPointerInfoEXT hostPtrInfo = {
					.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT,
					.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT,
					.pHostPointer = memAreaAdr
				};
				memAllocInfo.pNext = &hostPtrInfo;

				VkCheckRet(vkAllocateMemory(fDevice, &memAllocInfo, nullptr, &bindedBuffer.memory));
				VkCheckRet(vkBindImageMemory(fDevice, bindedBuffer.image, bindedBuffer.memory, 0));
				break;
			}
		}
	}
}
