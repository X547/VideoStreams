#include "TestFilter.h"


//#pragma mark - Consumer

void TestFilter::Consumer::Connected(bool isActive)
{
	VideoConsumer::Connected(isActive);
}

status_t TestFilter::Consumer::SwapChainRequested(const SwapChainSpec& spec)
{
	return Base().fProducer.RequestSwapChain(spec);
}

void TestFilter::Consumer::SwapChainChanged(bool isValid)
{
	VideoConsumer::SwapChainChanged(isValid);
}

void TestFilter::Consumer::Present(int32 bufferId, const BRegion* dirty)
{
	Base().fProducer.Present(bufferId, dirty);
}


//#pragma mark - Producer

void TestFilter::Producer::Connected(bool isActive)
{
	VideoProducer::Connected(isActive);
}

status_t TestFilter::Producer::SwapChainRequested(const SwapChainSpec& spec)
{
	return VideoProducer::SwapChainRequested(spec);
}

void TestFilter::Producer::SwapChainChanged(bool isValid)
{
	VideoProducer::SwapChainChanged(isValid);
	Base().fConsumer.SetSwapChain(isValid ? &GetSwapChain() : NULL);
}

void TestFilter::Producer::Presented(const PresentedInfo &presentedInfo)
{
	Base().fConsumer.Presented(presentedInfo);
}
