#ifndef _VIDEONODE_H_
#define _VIDEONODE_H_

#include <Handler.h>
#include <Messenger.h>
#include <private/shared/AutoDeleter.h>

class SwapChain;
class SwapChainSpec;


enum {
	videoNodeConnectMsg          = 1,
	videoNodeConnectedMsg        = 2,
	videoNodeRequestSwapChainMsg = 3,
	videoNodeSwapChainChangedMsg = 4,
	videoNodePresentMsg          = 5,
	videoNodePresentedMsg        = 6,
	videoNodeGetConsumerInfoMsg  = 7,

	videoNodeInternalLastMsg     = 31,

	videoNodeLastMsg             = 1023,
};


class _EXPORT VideoNode: public BHandler
{
private:
	bool fIsConnected;
	BMessenger fLink;
	bool fSwapChainValid, fOwnsSwapChain;
	ObjectDeleter<SwapChain> fSwapChain;

public:
	struct ConsumerInfo {
		int32 width;
		int32 height;
	};

	struct PresentedInfo {
		uint32 era;
		bool suboptimal;
		int32 width;
		int32 height;
	};
	
	VideoNode(const char* name = NULL);
	virtual ~VideoNode();

	bool IsConnected() const {return fIsConnected;}
	const BMessenger& Link() const {return fLink;}
	status_t ConnectTo(const BMessenger& link);
	virtual void Connected(bool isActive);

	bool SwapChainValid() const {return fSwapChainValid;}
	bool OwnsSwapChain() const {return fOwnsSwapChain;}
	const SwapChain& GetSwapChain() const {return *fSwapChain.Get();}
	status_t SetSwapChain(const SwapChain* swapChain);
	status_t RequestSwapChain(const SwapChainSpec& spec);
	virtual status_t SwapChainRequested(const SwapChainSpec& spec);
	virtual void SwapChainChanged(bool isValid);

	void MessageReceived(BMessage* msg) override;
};


void WriteMessenger(const BMessenger& obj);
void DumpSwapChain(const SwapChain &swapChain);


#endif	// _VIDEONODE_H_
