sources = [
	'VideoNode.cpp',
	'VideoProducer.cpp',
	'VideoConsumer.cpp',
]

libvideostreams = shared_library(
	'videostreams',
	sources,
	include_directories: [
		'/boot/system/develop/headers/private/app'
	],
	dependencies: [
		compiler.find_library('be')
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: 'lib'
)
