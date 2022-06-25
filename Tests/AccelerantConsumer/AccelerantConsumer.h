#pragma once

#include <stdio.h>

#include <VideoConsumer.h>

#include <Accelerant.h>


#define ACCELERANT_HOOK_LIST(REQUIRED, OPTIONAL) \
   REQUIRED(B_INIT_ACCELERANT, init_accelerant) \
   REQUIRED(B_UNINIT_ACCELERANT, uninit_accelerant) \
   OPTIONAL(B_CLONE_ACCELERANT, clone_accelerant) \
   OPTIONAL(B_PROPOSE_DISPLAY_MODE, propose_display_mode) \
   REQUIRED(B_GET_DISPLAY_MODE, get_display_mode) \
   REQUIRED(B_SET_DISPLAY_MODE, set_display_mode) \
   REQUIRED(B_DPMS_MODE, dpms_mode) \
   REQUIRED(B_SET_DPMS_MODE, set_dpms_mode) \
   REQUIRED(B_ACCELERANT_MODE_COUNT, accelerant_mode_count) \
   REQUIRED(B_GET_MODE_LIST, get_mode_list) \
   REQUIRED(B_GET_FRAME_BUFFER_CONFIG, get_frame_buffer_config) \
   REQUIRED(B_MOVE_DISPLAY, move_display_area) \
   OPTIONAL(B_ACCELERANT_RETRACE_SEMAPHORE, accelerant_retrace_semaphore)

struct AccelerantHooks {
	GetAccelerantHook get_accelerant_hook{};
#define DISPATCH_TABLE_ENTRY(id, type) type type##_hook{};
	ACCELERANT_HOOK_LIST(DISPATCH_TABLE_ENTRY, DISPATCH_TABLE_ENTRY)
#undef DISPATCH_TABLE_ENTRY

	bool Init()
	{
#define REQUIRED(id, type) type##_hook = reinterpret_cast<type>(get_accelerant_hook(id, NULL)); if (type##_hook == NULL) {fprintf(stderr, "[!] accelerant hook " #id " not found\n"); return false;}
#define OPTIONAL(id, type) type##_hook = reinterpret_cast<type>(get_accelerant_hook(id, NULL));
		ACCELERANT_HOOK_LIST(REQUIRED, OPTIONAL);
#undef REQUIRED
#undef OPTIONAL
		return true;
	}
};


class AccelerantConsumer final: public VideoConsumer
{
private:
	HandleDeleter<image_id, status_t, unload_add_on> fImage;

	AccelerantHooks fHooks;

	display_mode fDm;
	frame_buffer_config fFbc;
	
	thread_id fThread;
	bool fRun;
	int32 fQueueLen;
	sem_id fRetraceSem;
	
	status_t ConsumerThread();
	status_t SetupSwapChain();
	
public:
	AccelerantConsumer(const char* name);
	virtual ~AccelerantConsumer();
	status_t Init();

	void Connected(bool isActive) final;
	status_t SwapChainRequested(const SwapChainSpec& spec) final;
	void Present(int32 bufferId, const BRegion* dirty) final;
};
