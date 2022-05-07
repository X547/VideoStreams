sources = [
	'ProducerApp.cpp',
	'TestProducer.cpp',
	'AnimProducer.cpp',
]

TestProducer = executable(
	'TestProducer',
	sources,
	include_directories: [
		'/boot/system/develop/headers/private/app',
		'/boot/system/develop/headers/private/shared'
	],
	dependencies: [
		VideoStreams,
		compiler.find_library('be'),
		compiler.find_library('translation'),
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
