dep_libdrm = dependency('libdrm')
dep_libdrm_amdgpu = dependency('libdrm_amdgpu')


VulkanBind = executable(
	'VulkanBind',
	[
		'Main.cpp',
	],
	include_directories: [
		'/boot/system/develop/headers/private/app',
		'/boot/system/develop/headers/private/shared'
	],
	dependencies: [
		dep_libbe,
		dep_libvideostreams,
		dep_libvideostreams_vulkan,
		dep_libaccelerant,
		dep_libvulkan,
		dep_libtest_shared_vulkan,
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
