#include "TestProducer.h"

#include <stdio.h>


TestProducer::TestProducer(const char* name):
	VideoProducer(name),
	fSwapChainBind(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT),
	fRenderer(fVkDev)
{
	if (fVkDev.Init() < B_OK) {
			printf("[!] can't init Vulkan device\n");
			exit(1);
	}
	fSwapChainBind.SetDevice(fVkDev.GetPhysDevice(), fVkDev.GetDevice());
	if (fRenderer.Init() != VK_SUCCESS) {
			printf("[!] can't init Vulkan device\n");
			exit(1);
	}
}

TestProducer::~TestProducer()
{
}

void TestProducer::Connected(bool isActive)
{
	VideoProducer::Connected(isActive);
	if (isActive) {
		SwapChainSpec spec {
			.size = sizeof(SwapChainSpec),
			.presentEffect = presentEffectSwap,
			.bufferCnt = 2,
			.kind = bufferRefGpu,
			.width = 640,
			.height = 480,
			.colorSpace = B_RGBA32
		};
		if (RequestSwapChain(spec) < B_OK) {
			printf("[!] can't request swap chain\n");
			exit(1);
		}
	}
}

status_t TestProducer::SwapChainRequested(const SwapChainSpec& spec)
{
	return B_OK;
}

void TestProducer::SwapChainChanged(bool isValid)
{
	VideoProducer::SwapChainChanged(isValid);
	if (isValid) {
		DumpSwapChain(GetSwapChain());
		if (fSwapChainBind.ConnectTo(GetSwapChain()) < B_OK) {
			printf("[!] can't bind swap chain\n");
			return;
		}
		fSwapChainBind.Dump();
		fRenderer.ConnectTo(&fSwapChainBind);
		Produce();
	} else {
		fRenderer.ConnectTo(NULL);
	}
}


void TestProducer::Presented(const PresentedInfo &presentedInfo)
{
	VideoProducer::Presented(presentedInfo);
	Produce();
}


void TestProducer::Produce()
{
	int32 bufferId = AllocBuffer();
	fRenderer.Draw(bufferId);
	Present(bufferId);
}
