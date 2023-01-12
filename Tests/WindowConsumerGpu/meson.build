WindowConsumerGpu = executable(
	'WindowConsumerGpu',
	[
		'ConsumerApp.cpp',
		'ConsumerGpuView.cpp'
	],
	include_directories: [
		'/boot/system/develop/headers/private/app'
	],
	dependencies: [
		dep_libvideostreams,
		dep_libvideostreams_vulkan,
		dep_libtest_shared_vulkan,
		dep_libbe,
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
