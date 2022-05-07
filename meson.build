project('VideoStreams', 'cpp',
	version : '0.1',
	default_options : ['warning_level=3', 'cpp_std=c++2a']
)

add_project_arguments('-Wno-pedentic', language: 'cpp')
add_project_arguments('-Wno-missing-field-initializers', language: 'cpp')

compiler = meson.get_compiler('cpp')

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
	install_dir: 'lib'
)

VideoStreams = declare_dependency(
	link_with : VideoStreamsLib,
	include_directories: [
		'.',
		'/boot/system/develop/headers/private/app',
		'/boot/system/develop/headers/private/shared',
	]
)
