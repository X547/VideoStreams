VulkanGpuProducer = executable(
	'VulkanGpuProducer',
	[
		'ProducerApp.cpp',
		'TestProducer.cpp',
		'TestVulkanRenderer.cpp'
	],
	include_directories: [
		'/boot/system/develop/headers/private/app',
		'/boot/system/develop/headers/private/shared'
	],
	dependencies: [
		dep_libvideostreams,
		dep_libvideostreams_vulkan,
		dep_libbe,
		dep_libvulkan,
		dep_libtest_shared,
		dep_libtest_shared_vulkan,
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
