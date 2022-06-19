#include "VideoNode.h"
#include "VideoBuffer.h"
#include <private/app/MessengerPrivate.h>

#include <stdio.h>


inline int32 ReplaceError(int32 err, int32 replaceWith) {return err < B_OK ? replaceWith : err;}
#define CheckRet(err) {status_t _err = (err); if (_err < B_OK) {abort(); return _err;}}
#define CheckReply(msg, err) {status_t _err = (err); if (_err < B_OK) {BMessage reply(B_REPLY); reply.AddInt32("error", _err); msg->SendReply(&reply); return;}}


static status_t SendMessageSync(
	BHandler* src, const BMessenger &dst, BMessage* message, BMessage* reply,
	bigtime_t deliveryTimeout = B_INFINITE_TIMEOUT, bigtime_t replyTimeout = B_INFINITE_TIMEOUT
)
{
	if (dst.IsTargetLocal()) {
		BLooper* dstLooper;
		BHandler* dstHandler = dst.Target(&dstLooper);
		if (src->Looper() == dstLooper) {
			// !!! can't get reply
			dstHandler->MessageReceived(message);
			return B_OK;
		}
	}
	return dst.SendMessage(message, reply, deliveryTimeout, replyTimeout);
}

static status_t CopySwapChain(ObjectDeleter<SwapChain> &dst, const SwapChain &src)
{
	size_t numBytes = sizeof(SwapChain) + src.bufferCnt*sizeof(VideoBuffer);
	dst.SetTo((SwapChain*)::operator new(numBytes));
	if (!dst.IsSet()) return B_NO_MEMORY;
	memcpy(dst.Get(), &src, sizeof(SwapChain));
	dst->buffers = (VideoBuffer*)(dst.Get() + 1);
	memcpy(dst->buffers, src.buffers, src.bufferCnt*sizeof(VideoBuffer));

	return B_OK;
}

static status_t SetSwapChain(BMessage& msg, const SwapChain &swapChain)
{
	ObjectDeleter<SwapChain> swapChainCopy;
	CheckRet(CopySwapChain(swapChainCopy, swapChain));
	swapChainCopy->buffers = NULL;
	CheckRet(msg.AddData("swapChain", B_RAW_TYPE, swapChainCopy.Get(), sizeof(SwapChain) + swapChain.bufferCnt*sizeof(VideoBuffer)));

	return B_OK;
}

static status_t GetSwapChain(const BMessage& msg, ObjectDeleter<SwapChain> &swapChain)
{
	const void* data;
	ssize_t numBytes;

	CheckRet(msg.FindData("swapChain", B_RAW_TYPE, &data, &numBytes));
	if (numBytes < sizeof(SwapChain)) return B_BAD_VALUE;
	if (((SwapChain*)data)->size != sizeof(SwapChain)) return B_BAD_VALUE;
	if (numBytes != sizeof(SwapChain) + ((SwapChain*)data)->bufferCnt * sizeof(VideoBuffer)) return B_BAD_VALUE;
	swapChain.SetTo((SwapChain*)::operator new(numBytes));
	memcpy(swapChain.Get(), data, numBytes);
	swapChain->buffers = (VideoBuffer*)(swapChain.Get() + 1);

	return B_OK;
}


VideoNode::VideoNode(const char* name):
	BHandler(name),
	fIsConnected(false),
	fSwapChainValid(false),
	fOwnsSwapChain(false)
{
}

VideoNode::~VideoNode()
{
	if (ConnectTo(BMessenger()) < B_OK)
		printf("can't disconnect\n");
}


status_t VideoNode::ConnectTo(const BMessenger& link)
{
	printf("VideoNode::ConnectTo("); WriteMessenger(BMessenger(this)); printf(", "); WriteMessenger(link); printf(")\n");
	if (Link() == link)
		return B_OK;

	if (IsConnected()) {
		BMessage reply;
		BMessage msg(videoNodeConnectMsg);
		msg.AddBool("doConnect", false);
		CheckRet(SendMessageSync(this, Link(), &msg, &reply));
		fLink = BMessenger();
		fIsConnected = false;
		Connected(false);
	}
	if (link.IsValid()) {
/*
		struct LinkReleaser {
			VideoNode& base;
			bool doRelease;
			LinkReleaser(VideoNode& base): base(base), doRelease(true) {}
			void Detach() {doRelease = false;}
			~LinkReleaser()
			{
				if (doRelease) {
					base.fLink = BMessenger();
					base.fIsConnected = false;
				}
			}
		} linkReleaser(*this);
*/
		BMessage reply;
		BMessage msg(videoNodeConnectMsg);
		msg.AddBool("doConnect", true);
		msg.AddMessenger("link", BMessenger(this));
		CheckRet(SendMessageSync(this, link, &msg, &reply));
		int32 error;
		if (reply.FindInt32("error", &error) >= B_OK)
			CheckRet(error);

//		linkReleaser.Detach();
		fLink = link;
		fIsConnected = true;
		Connected(true);
	}
	return B_OK;
}

void VideoNode::Connected(bool isActive)
{}


status_t VideoNode::SetSwapChain(const SwapChain* swapChain)
{
	printf("VideoNode::SetSwapChain()\n");

	if (swapChain == NULL) {
		if (!SwapChainValid())
			return B_OK;
		if (!OwnsSwapChain())
			return B_NOT_ALLOWED;

		fSwapChain.Unset();
		fSwapChainValid = false;
		fOwnsSwapChain = false;

		if (IsConnected()) {
			BMessage msg(videoNodeSwapChainChangedMsg);
			msg.AddBool("isValid", false);
			BMessage reply;
			CheckRet(SendMessageSync(this, Link(), &msg, &reply));
		}

		SwapChainChanged(false);
		return B_OK;
	}

	if (swapChain->size != sizeof(SwapChain))
		return B_BAD_VALUE;
	
	CheckRet(CopySwapChain(fSwapChain, *swapChain));

	fSwapChainValid = true;
	fOwnsSwapChain = true;

	//printf("  swapChain: \n");
	//printf("    size: %" B_PRIuSIZE "\n", fSwapChain.size);
	//printf("    bufferCnt: %" B_PRIu32 "\n", fSwapChain.bufferCnt);

	if (IsConnected()) {
		BMessage msg(videoNodeSwapChainChangedMsg);
		msg.AddBool("isValid", true);
		CheckRet(::SetSwapChain(msg, *fSwapChain.Get()));

		BMessage reply;
		CheckRet(SendMessageSync(this, Link(), &msg, &reply));
	}

	SwapChainChanged(true);
	return B_OK;
}

status_t VideoNode::RequestSwapChain(const SwapChainSpec& spec)
{
	printf("VideoNode::RequestSwapChain()\n");
	printf("  Link(): "); WriteMessenger(Link()); printf("\n");

	if (!IsConnected())
		return B_ERROR;

	BMessage msg(videoNodeRequestSwapChainMsg);
	msg.AddData("spec", B_RAW_TYPE, &spec, sizeof(SwapChainSpec));
	Link().SendMessage(&msg);

	return B_OK;
}


status_t VideoNode::SwapChainRequested(const SwapChainSpec& spec)
{
	return B_NOT_ALLOWED;
}

void VideoNode::SwapChainChanged(bool isValid)
{
}


void VideoNode::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case videoNodeConnectMsg: {
		bool doConnect;
		CheckReply(msg, ReplaceError(msg->FindBool("doConnect", &doConnect), B_BAD_VALUE));
		if (doConnect) {
			if (IsConnected())
				CheckReply(msg, B_ERROR); // already connected
			CheckReply(msg, ReplaceError(msg->FindMessenger("link", &fLink), B_BAD_VALUE));
			if (!fLink.IsValid())
				CheckReply(msg, B_ERROR);
		} else {
			if (!IsConnected())
				CheckReply(msg, B_ERROR); // not connected
			fLink = BMessenger();
		}
		BMessage reply(B_REPLY);
		msg->SendReply(&reply);
		fIsConnected = doConnect;
		if (!IsConnected() && SwapChainValid() && !OwnsSwapChain()) {
			fSwapChain.Unset();
			fSwapChainValid = false;
			fOwnsSwapChain = false;
			SwapChainChanged(false);
		}
		Connected(doConnect);
		return;
	}
	case videoNodeRequestSwapChainMsg: {
		printf("videoNodeRequestSwapChainMsg\n");
		const void* data;
		ssize_t size;
		SwapChainSpec spec;
		CheckReply(msg, ReplaceError(msg->FindData("spec", B_RAW_TYPE, &data, &size), B_BAD_VALUE));
		if ((size_t)size < sizeof(SwapChainSpec))
			CheckReply(msg, B_BAD_VALUE);
		memcpy(&spec, data, sizeof(SwapChainSpec));
		CheckReply(msg, SwapChainRequested(spec));
		BMessage reply(B_REPLY);
		msg->SendReply(&reply);
		// SwapChainRequested(spec);
		return;
	}
	case videoNodeSwapChainChangedMsg: {
		printf("videoNodeSwapChainChangedMsg\n");
		bool isValid;
		CheckReply(msg, ReplaceError(msg->FindBool("isValid", &isValid), B_BAD_VALUE));
		if (isValid) {
			CheckReply(msg, ::GetSwapChain(*msg, fSwapChain));
			fSwapChainValid = true;
		} else {
			fSwapChain.Unset();
			fSwapChainValid = false;
		}

		SwapChainChanged(isValid);
		BMessage reply(B_REPLY);
		msg->SendReply(&reply);
	}
	}
	BHandler::MessageReceived(msg);
}


void _EXPORT WriteMessenger(const BMessenger& obj)
{
	printf("(team: %" B_PRId32 ", port: %" B_PRId32 ", token: %" B_PRId32 ")",
		BMessenger::Private((BMessenger*)&obj).Team(),
		BMessenger::Private((BMessenger*)&obj).Port(),
		BMessenger::Private((BMessenger*)&obj).Token()
	);
}

void _EXPORT DumpSwapChain(const SwapChain &swapChain)
{
	printf("swapChain: \n");
	printf("  size: %" B_PRIuSIZE "\n", swapChain.size);
	printf("  bufferCnt: %" B_PRIu32 "\n", swapChain.bufferCnt);
	printf("  buffers:\n");
	for (uint32 i = 0; i < swapChain.bufferCnt; i++) {
		const VideoBuffer &buffer = swapChain.buffers[i];
		printf("    %" B_PRIu32 "\n", i);
		switch (buffer.ref.kind) {
			case bufferRefArea: {
				printf("      ref.kind: area\n");
				printf("      ref.area.id: %" B_PRId32 "\n", buffer.ref.area.id);
				break;
			}
			case bufferRefGpu: {
				printf("      ref.kind: gpu\n");
				printf("      ref.area.id: %" B_PRId32 "\n",   buffer.ref.gpu.id);
				printf("      ref.area.team: %" B_PRId32 "\n", buffer.ref.gpu.team);
				break;
			}
			default:
				printf("      ref.kind: ?(%d)\n", buffer.ref.kind);
		}
		printf("      ref.offset: %" B_PRIuSIZE "\n",       buffer.ref.offset);
		printf("      ref.length: %" B_PRIu64 "\n",         buffer.ref.size);
		printf("      format.bytesPerRow: %" B_PRIu32 "\n", buffer.format.bytesPerRow);
		printf("      format.width: %" B_PRIu32 "\n",       buffer.format.width);
		printf("      format.height: %" B_PRIu32 "\n",      buffer.format.height);
		printf("      format.colorSpace: %d\n",             buffer.format.colorSpace);
	}
}
