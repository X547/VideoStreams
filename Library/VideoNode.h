#ifndef _VIDEONODE_H_
#define _VIDEONODE_H_

#include <Handler.h>
#include <Messenger.h>
#include <private/app/MessengerPrivate.h>
#include <private/shared/AutoDeleter.h>

#include <VideoBuffer.h>


enum {
	videoNodeConnectMsg          = 1,
	videoNodeRequestSwapChainMsg = 2,
	videoNodeSwapChainChangedMsg = 3,
	videoNodePresentMsg          = 4,
	videoNodePresentedMsg        = 5,

	videoNodeLastMsg             = 1023,
};


class _EXPORT VideoNode: public BHandler
{
private:
	bool fIsConnected;
	BMessenger fLink;
	bool fSwapChainValid, fOwnsSwapChain;
	SwapChain fSwapChain;
	ArrayDeleter<VideoBuffer> fBuffers;

public:
	VideoNode(const char* name = NULL);
	virtual ~VideoNode();
	
	bool IsConnected() const {return fIsConnected;}
	const BMessenger& Link() const {return fLink;}
	status_t ConnectTo(const BMessenger& link);
	virtual void Connected(bool isActive);

	bool SwapChainValid() const {return fSwapChainValid;}
	bool OwnsSwapChain() const {return fOwnsSwapChain;}
	const SwapChain& GetSwapChain() const {return fSwapChain;}
	status_t SetSwapChain(const SwapChain* swapChain);
	status_t RequestSwapChain(const SwapChainSpec& spec);
	virtual status_t SwapChainRequested(const SwapChainSpec& spec);
	virtual void SwapChainChanged(bool isValid);

	virtual void MessageReceived(BMessage* msg);
};


static void WriteMessenger(const BMessenger& obj)
{
	printf("(team: %" B_PRId32 ", port: %" B_PRId32 ", token: %" B_PRId32 ")",
		BMessenger::Private((BMessenger*)&obj).Team(),
		BMessenger::Private((BMessenger*)&obj).Port(),
		BMessenger::Private((BMessenger*)&obj).Token()
	);
}


#endif	// _VIDEONODE_H_
