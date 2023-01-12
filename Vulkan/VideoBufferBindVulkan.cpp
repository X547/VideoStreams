#include "VideoBufferBindVulkan.h"
#include "VideoBuffer.h"

#include <stdio.h>
#include <assert.h>

#define VkCheckRet(err) { \
	VkResult _err = (err); \
	if (_err != VK_SUCCESS) { \
		fprintf(stderr, "[!] %s:%d: Vulkan error %d\n", __FILE__, __LINE__, _err); \
		abort(); \
	} \
} \


static uint32_t GetMemoryTypeIndex(VkPhysicalDevice physDev, uint32_t typeBits, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties props;
	vkGetPhysicalDeviceMemoryProperties(physDev, &props);
	for (unsigned i = 0u; i < props.memoryTypeCount; ++i) {
		if (typeBits & (1 << i)) {
			if ((props.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
	}
	return 0;
}


SwapChainBindVulkan::SwapChainBindVulkan(VkImageUsageFlags imageUsage): fImageUsage(imageUsage)
{
}

SwapChainBindVulkan::~SwapChainBindVulkan()
{
	Unset();
}

void SwapChainBindVulkan::SetDevice(VkPhysicalDevice physDev, VkDevice dev)
{
	printf("SetDevice(physDev: %p, dev: %p)\n", physDev, dev);
	fPhysDev = physDev;
	fDevice = dev;

	fFns.GetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(fDevice, "vkGetMemoryFdKHR");
	if (fFns.GetMemoryFdKHR == NULL) fprintf(stderr, "[!] vkGetDeviceProcAddr: procedure vkGetMemoryFdKHR not found\n");
	fFns.GetMemoryFdPropertiesKHR = (PFN_vkGetMemoryFdPropertiesKHR)vkGetDeviceProcAddr(fDevice, "vkGetMemoryFdPropertiesKHR");
	if (fFns.GetMemoryFdPropertiesKHR == NULL) fprintf(stderr, "[!] vkGetDeviceProcAddr: procedure vkGetMemoryFdPropertiesKHR not found\n");
}

void SwapChainBindVulkan::Unset()
{
	for (uint32 i = 0; i < fBufferCnt; i++) {
		auto &bindedBuffer = fBindedBuffers[i];
		vkDestroyImage(fDevice, bindedBuffer.image, NULL);
		vkFreeMemory(fDevice, bindedBuffer.memory, NULL);
	}
	fBindedBuffers.Unset();
	fBufferCnt = 0;
}

status_t SwapChainBindVulkan::ConnectTo(const SwapChain &swapChain)
{
	Unset();
	fBufferCnt = swapChain.bufferCnt;
	fBindedBuffers.SetTo(new BindedBuffer[swapChain.bufferCnt]);
	for (uint32 i = 0; i < swapChain.bufferCnt; i++) {
		const auto &buffer = swapChain.buffers[i];
		auto &bindedBuffer = fBindedBuffers[i];

		VkImageCreateInfo createInfo {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
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
			.usage = fImageUsage,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};

		switch (buffer.ref.kind) {
			case bufferRefArea: {
				VkCheckRet(vkCreateImage(fDevice, &createInfo, NULL, &bindedBuffer.image));

				VkMemoryRequirements memRequirements;
				vkGetImageMemoryRequirements(fDevice, bindedBuffer.image, &memRequirements);
				size_t memTypeIdx = GetMemoryTypeIndex(fPhysDev, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

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
			case bufferRefGpu: {
				VkExternalMemoryImageCreateInfo extMemImgInfo = {
					.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
					.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
				};
				createInfo.pNext = &extMemImgInfo;
				VkCheckRet(vkCreateImage(fDevice, &createInfo, NULL, &bindedBuffer.image));

				VkMemoryFdPropertiesKHR fdp = {
					.sType = VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR,
				};
				VkCheckRet(fFns.GetMemoryFdPropertiesKHR(fDevice, VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT, buffer.ref.gpu.fd, &fdp));

				VkImageMemoryRequirementsInfo2 memri = {
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
					.image = bindedBuffer.image
				};
				VkMemoryRequirements2 memr = {
					.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
				};
				vkGetImageMemoryRequirements2(fDevice, &memri, &memr);
				uint32_t mem = GetMemoryTypeIndex(fPhysDev, memr.memoryRequirements.memoryTypeBits & fdp.memoryTypeBits, 0);

				VkMemoryAllocateInfo memAllocInfo = {
					.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
					.allocationSize = memr.memoryRequirements.size,
					.memoryTypeIndex = mem,
				};
				VkMemoryDedicatedAllocateInfo dedicateInfo {
					.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
					.image = bindedBuffer.image,
				};
				memAllocInfo.pNext = &dedicateInfo;
				VkImportMemoryFdInfoKHR memFdInfo = {
					.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
					.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
					.fd = dup(buffer.ref.gpu.fd),
				};
				dedicateInfo.pNext = &memFdInfo;
				VkMemoryDedicatedAllocateInfo dedi = {
					.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
					.image = bindedBuffer.image,
				};
				memFdInfo.pNext = &dedi;
				VkCheckRet(vkAllocateMemory(fDevice, &memAllocInfo, NULL, &bindedBuffer.memory));
				VkCheckRet(vkBindImageMemory(fDevice, bindedBuffer.image, bindedBuffer.memory, 0));
				break;
			}
		}

		bindedBuffer.imageInfo = createInfo;
		bindedBuffer.imageInfo.pNext = NULL;
	}
	return B_OK;
}

status_t SwapChainBindVulkan::Alloc(ObjectDeleter<SwapChain> &outSwapChain, const SwapChainSpec &spec)
{
	Unset();

	ObjectDeleter<SwapChain> swapChain(SwapChain::New(spec.bufferCnt));
	if (!swapChain.IsSet()) return B_NO_MEMORY;
	swapChain->presentEffect = spec.presentEffect;

	fBindedBuffers.SetTo(new BindedBuffer[spec.bufferCnt]);
	fBufferCnt = spec.bufferCnt;
	for (uint32 i = 0; i < spec.bufferCnt; i++) {
		auto &buffer = swapChain->buffers[i];
		auto &bindedBuffer = fBindedBuffers[i];

		VkImageCreateInfo createInfo {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_B8G8R8A8_UNORM, // !!!
			.extent = {
				spec.width,
				spec.height,
				1
			},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_LINEAR,
			.usage = fImageUsage,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};
		VkCheckRet(vkCreateImage(fDevice, &createInfo, NULL, &bindedBuffer.image));

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(fDevice, bindedBuffer.image, &memRequirements);

		size_t memTypeIdx = 0;
		for (; memTypeIdx < 8 * sizeof(memRequirements.memoryTypeBits); ++memTypeIdx) {
			if (memRequirements.memoryTypeBits & (1u << memTypeIdx))
				break;
		}
		assert(memTypeIdx <= 8 * sizeof(memRequirements.memoryTypeBits) - 1);

		VkMemoryAllocateInfo memAllocInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = (uint32_t)memTypeIdx
		};
		VkMemoryDedicatedAllocateInfo dedi = {
			.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
			.image = bindedBuffer.image,
		};
		memAllocInfo.pNext = &dedi;
		VkExportMemoryAllocateInfo memExportInfo {
			.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
			.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
		};
		dedi.pNext = &memExportInfo;

		VkCheckRet(vkAllocateMemory(fDevice, &memAllocInfo, NULL, &bindedBuffer.memory));
		VkCheckRet(vkBindImageMemory(fDevice, bindedBuffer.image, bindedBuffer.memory, 0));

		buffer = {
			.id = i,
			.ref = {
				.size = memRequirements.size,
				.kind = bufferRefGpu
			},
			.format = {
				.width = spec.width,
				.height = spec.height,
				.colorSpace = spec.colorSpace
			}
		};
		buffer.format.bytesPerRow = 4*buffer.format.width; // !!!
		buffer.ref.gpu.fd = -1;
		buffer.ref.gpu.fenceFd = -1;

		VkMemoryGetFdInfoKHR fdInfo {
			.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR,
			.memory = bindedBuffer.memory,
			.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT
		};
		VkCheckRet(fFns.GetMemoryFdKHR(fDevice, &fdInfo, &buffer.ref.gpu.fd));

		bindedBuffer.imageInfo = createInfo;
		bindedBuffer.imageInfo.pNext = NULL;
	}

	outSwapChain.SetTo(swapChain.Detach());
	return B_OK;
}

SwapChainBindVulkan::BindedBuffer &SwapChainBindVulkan::BufferAt(size_t idx)
{
	return fBindedBuffers[idx];
}

void SwapChainBindVulkan::Dump()
{
	printf("bindedBuffers:\n");
	for (uint32 i = 0; i < fBufferCnt; i++) {
		const SwapChainBindVulkan::BindedBuffer &buffer = fBindedBuffers[i];
		printf("  %" B_PRIu32 "\n", i);
		printf("    image: %p\n", buffer.image);
		printf("    memory: %p\n", buffer.memory);
	}
}
