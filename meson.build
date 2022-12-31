project('VideoStreams', 'cpp',
	version : '0.1',
	default_options : ['warning_level=3', 'cpp_std=c++2a']
)

add_project_arguments('-Wno-pedantic', language: 'cpp')
add_project_arguments('-Wno-missing-field-initializers', language: 'cpp')

compiler = meson.get_compiler('cpp')
pkg = import('pkgconfig')

sources = [
	'VideoNode.cpp',
	'VideoProducer.cpp',
	'VideoConsumer.cpp',
	'VideoBufferUtils.cpp',
	'CompositeProducer.cpp',
	'CompositeConsumer.cpp',
	'CompositeProxy.cpp',
	'RasBuf.cpp',
	'AreaCloner.cpp',
	'TestProducerBase.cpp',
	'BufferQueue.cpp',
	'VideoBufferBindSW.cpp',
	'VideoBufferBindBitmap.cpp',
	'ConsumerView.cpp',
]

VideoStreamsLib = shared_library(
	'videostreams',
	sources,
	include_directories: [
		'/boot/system/develop/headers/private/app',
		'/boot/system/develop/headers/private/shared'
	],
	dependencies: [
		compiler.find_library('be')
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
)

VideoStreams = declare_dependency(
	link_with : VideoStreamsLib,
	include_directories: [
		'.',
		'/boot/system/develop/headers/private/app',
		'/boot/system/develop/headers/private/shared',
	]
)

install_headers(
	[
		'VideoNode.h',
		'VideoProducer.h',
		'VideoConsumer.h',
		'VideoBuffer.h',
		'BufferQueue.h',
		'TestProducerBase.h',
		'VideoBufferBindSW.h',
		'VideoBufferBindBitmap.h',
		'AreaCloner.h',
		'RasBuf.h',
		'CompositeProducer.h',
		'CompositeConsumer.h',
		'CompositeProxy.h',
		'ConsumerView.h',
	],
	subdir: 'VideoStreams'
)

install_data(
	'.PackageInfo',
	install_dir: '.'
)

pkg.generate(
  VideoStreamsLib,
  name: 'libvideostreams',
  subdirs: ['VideoStreams'],
  description: 'VideoStreams',
)


subdir('Vulkan')
