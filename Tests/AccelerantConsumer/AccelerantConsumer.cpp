#include "AccelerantConsumer.h"
#include "VideoBuffer.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <FindDirectory.h>
#include <graphic_driver.h>
#include <Screen.h>
#include <Looper.h>
#include <String.h>

#include <AppServerLink.h>
#include <ServerProtocol.h>
#include <private/shared/AutoDeleter.h>

#include "AutoLocker.h"

using BPrivate::AppServerLink;


#define CheckRet(err) {status_t _err = (err); if (_err < B_OK) return _err;}


static int pick_device(const char *apath) {
	DIR				*d;
	struct dirent	*e;
	char name_buf[1024];
	int fd = -1;

	/* open directory apath */
	d = opendir(apath);
	if (!d) return B_ERROR;
	/* get a list of devices, filtering out ".", "..", and "stub" */
	/* the only reason stub is disabled is that I know stub (aka R3-style) drivers don't support wait for retrace */
	while ((e = readdir(d)) != NULL) {
		if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..") || !strcmp(e->d_name, "stub"))
			continue;
		strcpy(name_buf, apath);
		strcat(name_buf, "/");
		strcat(name_buf, e->d_name);
		fd = open(name_buf, B_READ_WRITE);
		if (fd >= 0) {
			char signature[1024];
			if (ioctl(fd, B_GET_ACCELERANT_SIGNATURE, &signature, sizeof(signature)) >= B_OK) {
				if (strcmp(signature, "radeon_hd.accelerant") == 0) break;
			}
			close(fd);
		}
	}
	closedir(d);
	return fd;
}


static image_id load_accelerant(int fd, AccelerantHooks &hooks) {
	status_t result;
	image_id image = -1;
	unsigned i;
	char
		signature[1024],
		path[PATH_MAX];
	struct stat st;
	const static directory_which vols[] = {
		B_USER_NONPACKAGED_ADDONS_DIRECTORY,
		B_USER_ADDONS_DIRECTORY,
		B_SYSTEM_ADDONS_DIRECTORY,
	};

	/* get signature from driver */
	result = ioctl(fd, B_GET_ACCELERANT_SIGNATURE, &signature, sizeof(signature));
	if (result != B_OK) goto done;
	printf("B_GET_ACCELERANT_SIGNATURE returned ->%s<-\n", signature);

	// note failure by default
	for(i=0; i < sizeof (vols) / sizeof (vols[0]); i++) {

		/* ---
			compute directory path to common or beos addon directory on
			floppy or boot volume
		--- */

		printf("attempting to get path for %d (%d)\n", i, vols[i]);
		if (find_directory (vols[i], -1, false, path, PATH_MAX) != B_OK) {
			printf("find directory failed\n");
			continue;
		}

		strcat (path, "/accelerants/");
		strcat (path, signature);

		printf("about to stat(%s)\n", path);
		// don't try to load non-existant files
		if (stat(path, &st) != 0) continue;
		printf("Trying to load accelerant: %s\n", path);
		// load the image
		image = load_add_on(path);
		if (image >= 0) {
			printf("Accelerant loaded!\n");
			// get entrypoint from accelerant
			result = get_image_symbol(image, B_ACCELERANT_ENTRY_POINT,
#if defined(__INTEL__)
				B_SYMBOL_TYPE_ANY,
#else
				B_SYMBOL_TYPE_TEXT,
#endif
				(void **)&hooks.get_accelerant_hook
			);
			if (result == B_OK) {
				printf("Entry point %s() found: %p\n", B_ACCELERANT_ENTRY_POINT, hooks.get_accelerant_hook);
				if (hooks.Init() && ((result = hooks.init_accelerant_hook(fd)) == B_OK)) {
					// we have a winner!
					printf("Accelerant %s accepts the job!\n", path);
					break;
				} else {
					printf("init_accelerant refuses the the driver: %" B_PRId32 "\n", result);
				}
			} else {
				printf("Couldn't find the entry point :-(\n");
			}
			// unload the accelerant, as we must be able to init!
			unload_add_on(image);
		}
		if (image < 0) printf("image failed to load with reason %#" B_PRIx32 " (%s)\n", image, strerror(image));
		// mark failure to load image
		image = -1;
	}

	printf("Add-on image id: %" B_PRId32 "\n", image);

done:
	return image;
}


static image_id load_accelerant_primary(AccelerantHooks &hooks)
{
	/* find a graphic device to open */
	FileDescriptorCloser fd(pick_device("/dev/graphics"));
	if (!fd.IsSet()) {
		printf("Can't open device: %s (%s)\n", strerror(fd.Get()), strerror(errno));
		return errno;
	}
	/* load the accelerant */
	image_id image = load_accelerant(fd.Get(), hooks);
	CheckRet(image);
	fd.Detach();
	return image;
}

static image_id load_accelerant_clone(AccelerantHooks &hooks)
{
	HandleDeleter<image_id, status_t, unload_add_on> fAddonImage;

	BScreen screen;

	AppServerLink link;
	link.StartMessage(AS_GET_ACCELERANT_PATH);
	link.Attach<screen_id>(screen.ID());

	status_t status = B_ERROR;
	if (link.FlushWithReply(status) < B_OK || status < B_OK)
		return status;

	BString accelerantPath;
	link.ReadString(accelerantPath);

	link.StartMessage(AS_GET_DRIVER_PATH);
	link.Attach<screen_id>(screen.ID());

	status = B_ERROR;
	if (link.FlushWithReply(status) < B_OK || status < B_OK)
		return status;

	BString driverPath;
	link.ReadString(driverPath);
	printf("driverPath: %s\n", driverPath.String());

	fAddonImage.SetTo(load_add_on(accelerantPath.String() /*"/Haiku/data/packages/haiku2/generated.x86_64/objects/haiku/x86_64/release/add-ons/accelerants/intel_extreme/intel_extreme.accelerant"*/));
	if (!fAddonImage.IsSet()) {
		fprintf(stderr, "InitClone: cannot load accelerant image\n");
		return fAddonImage.Get();
	}

	status = get_image_symbol(fAddonImage.Get(), B_ACCELERANT_ENTRY_POINT,
		B_SYMBOL_TYPE_TEXT, (void**)&hooks.get_accelerant_hook);
	if (status < B_OK) {
		fprintf(stderr, "InitClone: cannot get accelerant entry point\n");
		return B_NOT_SUPPORTED;
	}
	
	if (!hooks.Init())
		return B_NOT_SUPPORTED;

	status = hooks.clone_accelerant_hook((void*)driverPath.String());
	if (status < B_OK) {
		fprintf(stderr, "InitClone: cannot clone accelerant: %s\n", strerror(status));
		return status;
	}

	return fAddonImage.Detach();
}


static const char *spaceToString(uint32 cs) {
	const char *s;
	switch (cs) {
#define s2s(a) case a: s = #a ; break
		s2s(B_RGB32);
		s2s(B_RGBA32);
		s2s(B_RGB32_BIG);
		s2s(B_RGBA32_BIG);
		s2s(B_RGB16);
		s2s(B_RGB16_BIG);
		s2s(B_RGB15);
		s2s(B_RGBA15);
		s2s(B_RGB15_BIG);
		s2s(B_RGBA15_BIG);
		s2s(B_CMAP8);
		s2s(B_GRAY8);
		s2s(B_GRAY1);
		s2s(B_YCbCr422);
		s2s(B_YCbCr420);
		s2s(B_YUV422);
		s2s(B_YUV411);
		s2s(B_YUV9);
		s2s(B_YUV12);
		default:
			s = "unknown"; break;
#undef s2s
	}
	return s;
}

static void dump_mode(display_mode *dm) {
	display_timing *t = &(dm->timing);
	printf("  pixel_clock: %" B_PRId32 "KHz\n", t->pixel_clock);
	printf("            H: %4d %4d %4d %4d\n", t->h_display, t->h_sync_start, t->h_sync_end, t->h_total);
	printf("            V: %4d %4d %4d %4d\n", t->v_display, t->v_sync_start, t->v_sync_end, t->v_total);
	printf(" timing flags:");
	if (t->flags & B_BLANK_PEDESTAL) printf(" B_BLANK_PEDESTAL");
	if (t->flags & B_TIMING_INTERLACED) printf(" B_TIMING_INTERLACED");
	if (t->flags & B_POSITIVE_HSYNC) printf(" B_POSITIVE_HSYNC");
	if (t->flags & B_POSITIVE_VSYNC) printf(" B_POSITIVE_VSYNC");
	if (t->flags & B_SYNC_ON_GREEN) printf(" B_SYNC_ON_GREEN");
	if (!t->flags) printf(" (none)\n");
	else printf("\n");
	printf(" refresh rate: %4.2f\n", ((double)t->pixel_clock * 1000) / ((double)t->h_total * (double)t->v_total));
	printf("  color space: %s\n", spaceToString(dm->space));
	printf(" virtual size: %dx%d\n", dm->virtual_width, dm->virtual_height);
	printf("dispaly start: %d,%d\n", dm->h_display_start, dm->v_display_start);

	printf("   mode flags:");
	if (dm->flags & B_SCROLL) printf(" B_SCROLL");
	if (dm->flags & B_8_BIT_DAC) printf(" B_8_BIT_DAC");
	if (dm->flags & B_HARDWARE_CURSOR) printf(" B_HARDWARE_CURSOR");
	if (dm->flags & B_PARALLEL_ACCESS) printf(" B_PARALLEL_ACCESS");
//	if (dm->flags & B_SUPPORTS_OVERLAYS) printf(" B_SUPPORTS_OVERLAYS");
	if (!dm->flags) printf(" (none)\n");
	else printf("\n");
}

static status_t get_and_set_mode(const AccelerantHooks &hooks, display_mode *dm) {
	uint32 mode_count;
	ArrayDeleter<display_mode> mode_list;
	display_mode target, high, low;
	status_t result = B_ERROR;
	uint32 i;

	mode_count = hooks.accelerant_mode_count_hook();
	printf("mode_count = %" B_PRId32 "\n", mode_count);
	if (mode_count == 0) return result;

	mode_list.SetTo(new display_mode[mode_count]);
	if (!mode_list.IsSet()) {
		printf("Couldn't calloc() for mode list\n");
		return result;
	}
	if (hooks.get_mode_list_hook(mode_list.Get()) != B_OK) {
		printf("mode list retrieval failed\n");
		return result;
	}

	/* take the first mode in the list */
//	target = high = low = *mode_list;

	for (i = 0; i < mode_count; i++) {
		if (
			((mode_list[i].space & ~0x3000) == B_RGB32_LITTLE || (mode_list[i].space & ~0x3000) == B_RGB32_BIG) &&
			mode_list[i].timing.h_display == 1920 && mode_list[i].timing.v_display == 1080 &&
			((double)mode_list[i].timing.pixel_clock * 1000) / ((double)mode_list[i].timing.h_total * (double)mode_list[i].timing.v_total) == 60
		) {
			target = high = low = mode_list[i];
			break;
		}
	}

	/* make as tall a virtual height as possible */
	//target.virtual_width = high.virtual_width = 2*target.virtual_width;
	target.virtual_height = high.virtual_height = 2*target.virtual_height;
	/* propose the display mode */
	if (hooks.propose_display_mode_hook == NULL || hooks.propose_display_mode_hook(&target, &low, &high) == B_ERROR) {
		printf("propose_display_mode failed\n");
		//goto free_mode_list;
	}
	printf("Target display mode: \n");
	dump_mode(&target);
	//exit(1);
	/* we got a display mode, now set it */
	//BScreen screen;
	if (hooks.set_display_mode_hook(&target) == B_ERROR /*screen.SetMode(&target) == B_ERROR*/) {
		printf("set display mode failed\n");
		return result;
	}
	hooks.set_dpms_mode_hook(B_DPMS_ON);
	//screen.SetDPMS(B_DPMS_ON);
	display_mode curMode;
	hooks.get_display_mode_hook(&curMode);
	printf("Current display mode: \n");
	dump_mode(&target);

	/* note the mode and success */
	*dm = target;
	result = B_OK;
	
	return result;
}


status_t AccelerantConsumer::ConsumerThread()
{
	for (;;) {
		// printf("ConsumerThread: step\n");
		while (acquire_sem(fRetraceSem) == B_INTERRUPTED) {}
		AutoLocker<AccelerantConsumer, AutoLockerHandlerLocking<AccelerantConsumer> > lock(this);
		if (!fRun) break;
		if (fQueueLen > 0) {
			// printf("ConsumerThread: present %d\n", c.DisplayBufferId());
			fQueueLen--;
			fHooks.move_display_area_hook(0, fDm.timing.v_display*DisplayBufferId());
			PresentedInfo presentedInfo {};
			Presented(presentedInfo);
		}
	}
	return B_OK;
}

status_t AccelerantConsumer::SetupSwapChain()
{
	area_id framebufferArea = area_for(fFbc.frame_buffer);
	CheckRet(framebufferArea);
	area_info areaInfo;
	CheckRet(get_area_info(framebufferArea, &areaInfo));
	void* framebufferAreaBase = areaInfo.address;

	SwapChain swapChain;
	swapChain.size = sizeof(SwapChain);
	swapChain.presentEffect = presentEffectSwap;
	swapChain.bufferCnt = 2;
	ArrayDeleter<VideoBuffer> buffers(new VideoBuffer[2]);
	swapChain.buffers = buffers.Get();
	for (uint32 i = 0; i < swapChain.bufferCnt; i++) {
		buffers[i].id = i;
		buffers[i].ref.kind           = bufferRefArea;
		buffers[i].ref.area.id        = framebufferArea;
		buffers[i].ref.offset         = (addr_t)fFbc.frame_buffer - (addr_t)framebufferAreaBase +
			i * fFbc.bytes_per_row * fDm.timing.v_display;
		buffers[i].ref.size           = fFbc.bytes_per_row * fDm.timing.v_display;
		buffers[i].format.bytesPerRow = fFbc.bytes_per_row;
		buffers[i].format.width       = fDm.timing.h_display;
		buffers[i].format.height      = fDm.timing.v_display;
		buffers[i].format.colorSpace  = B_RGBA32;
	}
	SetSwapChain(&swapChain);
	return B_OK;
}


AccelerantConsumer::AccelerantConsumer(const char* name): VideoConsumer(name),
	fThread(-1)
{
}

AccelerantConsumer::~AccelerantConsumer()
{
	printf("-AccelerantConsumer: "); WriteMessenger(BMessenger(this)); printf("\n");

	if (fThread >= B_OK) {
		status_t res;
		suspend_thread(fThread);
		fRun = false;
		int32 lockCnt = Looper()->CountLocks();
		for (int32 i = 0; i < lockCnt; i++) UnlockLooper();
		wait_for_thread(fThread, &res);
		for (int32 i = 0; i < lockCnt; i++) LockLooper();
		fThread = -1;
	}

	if (fImage.IsSet()) {
		fHooks.move_display_area_hook(0, 0);
		if (false && fHooks.uninit_accelerant_hook != NULL)
			fHooks.uninit_accelerant_hook();
	}
}

status_t AccelerantConsumer::Init()
{
	if (true) {
		fImage.SetTo(load_accelerant_primary(fHooks));
	} else {
		fImage.SetTo(load_accelerant_clone(fHooks));
	}
	CheckRet(fImage.Get());
	CheckRet(get_and_set_mode(fHooks, &fDm));

	fRetraceSem = (fHooks.accelerant_retrace_semaphore_hook == NULL) ? -1 : fHooks.accelerant_retrace_semaphore_hook();
	fHooks.get_frame_buffer_config_hook(&fFbc);
/*
	fRun = true;
	fThread = spawn_thread([](void* arg) {return ((AccelerantConsumer*)arg)->ConsumerThread();}, "consumerThread", B_NORMAL_PRIORITY, this);
	resume_thread(fThread);
*/
	return B_OK;
}

void AccelerantConsumer::Connected(bool isActive)
{
	if (isActive) {
		printf("AccelerantConsumer: connected to ");
		WriteMessenger(Link());
		printf("\n");
	} else {
		printf("AccelerantConsumer: disconnected\n");
		SetSwapChain(NULL);
	}
}

status_t AccelerantConsumer::SwapChainRequested(const SwapChainSpec& spec)
{
	printf("AccelerantConsumer::SwapChainRequested(%" B_PRIuSIZE ")\n", spec.bufferCnt);
	SetupSwapChain();
	return B_OK;
}

void AccelerantConsumer::Present(const BRegion* dirty)
{
	fQueueLen++;
	//fHooks.move_display_area_hook(0, fDm.timing.v_display*DisplayBufferId());
	//Presented();
}
