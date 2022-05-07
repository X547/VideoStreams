sources = [
	'ProducerApp.cpp',
	'VideoFileProducer.cpp',
]

VideoFileProducer = executable(
	'VideoFileProducer',
	sources,
	include_directories: [
		'/boot/system/develop/headers/private/app',
		'/boot/system/develop/headers/private/shared'
	],
	dependencies: [
		VideoStreams,
		compiler.find_library('be'),
		compiler.find_library('media'),
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
