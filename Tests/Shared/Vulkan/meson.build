libtest_shared_vulkan = static_library(
	'test_shared_vulkan',
	[
		'VulkanDevice.cpp',
	],
	include_directories: [
		'/boot/system/develop/headers/private/shared'
	],
	dependencies: [
		dep_libvulkan,
	],
	gnu_symbol_visibility: 'hidden',
)

dep_libtest_shared_vulkan = declare_dependency(
	link_with: libtest_shared_vulkan,
	dependencies: [
		dep_libvulkan,
	],
	include_directories: [
		'.'
	],
)
