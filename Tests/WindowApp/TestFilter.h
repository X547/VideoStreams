#pragma once

#include <VideoConsumer.h>
#include <VideoProducer.h>


class TestFilter {
private:
	class Consumer: public VideoConsumer {
	private:
		inline TestFilter &Base() {return *(TestFilter*)((char*)this - offsetof(TestFilter, fConsumer));}

	public:
		virtual ~Consumer() {}
		
		void Connected(bool isActive) final;
		status_t SwapChainRequested(const SwapChainSpec& spec) final;
		void SwapChainChanged(bool isValid) final;

		void Present(int32 bufferId, const BRegion* dirty) final;
	} fConsumer;

	class Producer: public VideoProducer {
	private:
		inline TestFilter &Base() {return *(TestFilter*)((char*)this - offsetof(TestFilter, fProducer));}

	public:
		virtual ~Producer() {}
		
		void Connected(bool isActive) final;
		status_t SwapChainRequested(const SwapChainSpec& spec) final;
		void SwapChainChanged(bool isValid) final;

		void Presented(const PresentedInfo &presentedInfo) final;
	} fProducer;

public:
	VideoConsumer *GetConsumer() {return &fConsumer;}
	VideoProducer *GetProducer() {return &fProducer;}
};
