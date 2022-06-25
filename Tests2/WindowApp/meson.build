project('WindowApp', 'cpp',
	version : '0.1',
	default_options : ['warning_level=3', 'cpp_std=c++2a']
)

compiler = meson.get_compiler('cpp')


sources = [
	'ConsumerApp.cpp',
	'TestProducer.cpp',
	'TestFilter.cpp',
]

VideoStreamsProj = subproject('VideoStreams')
VideoStreams = VideoStreamsProj.get_variable('VideoStreams')
#VideoStreams = dependency('libvideostreams')

WindowApp = executable(
	'WindowApp',
	sources,
	include_directories: [
		'/boot/system/develop/headers/private/shared'
	],
	dependencies: [
		VideoStreams,
		compiler.find_library('be')
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
