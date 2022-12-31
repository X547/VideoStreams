sources = [
	'VideoBufferBindVulkan.cpp',
	'VulkanDevice.cpp',
]

VideoStreamsVulkanLib = shared_library(
	'videostreams_vulkan',
	sources,
	include_directories: [
		'/boot/system/develop/headers/private/app',
		'/boot/system/develop/headers/private/shared'
	],
	dependencies: [
		compiler.find_library('be'),
		dependency('vulkan'),
		dependency('libdrm'),
		VideoStreams
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
)

VideoStreamsVulkan = declare_dependency(
	link_with : VideoStreamsVulkanLib,
	include_directories: [
		'/boot/system/develop/headers/private/shared',
	]
)

install_headers(
	[
		'VideoBufferBindVulkan.h',
	],
	subdir: 'VideoStreams'
)

install_symlink('libvideostreams_vulkan.so',
    pointing_to: '../../lib/libvideostreams_vulkan.so',
    install_dir: 'develop/lib')

pkg.generate(
  VideoStreamsVulkanLib,
  name: 'libvideostreams_vulkan',
  subdirs: ['VideoStreams'],
  description: 'VideoStreams Vulkan support',
  install_dir: 'develop/lib/pkgconfig'
)
