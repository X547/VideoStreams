#ifndef _TESTPRODUCER_H_
#define _TESTPRODUCER_H_

#include <VideoProducer.h>
#include <VideoBufferBindVulkan.h>

#include <VulkanDevice.h>

#include "TestVulkanRenderer.h"


class TestProducer final: public VideoProducer
{
private:
	VulkanDevice fVkDev;
	SwapChainBindVulkan fSwapChainBind;
	TestVulkanRenderer fRenderer;

	void Produce();

public:
	TestProducer(const char* name);
	virtual ~TestProducer();

	void Connected(bool isActive) final;
	status_t SwapChainRequested(const SwapChainSpec& spec) final;
	void SwapChainChanged(bool isValid) final;

	void Presented(const PresentedInfo &presentedInfo) final;
};


#endif	// _TESTPRODUCER_H_
