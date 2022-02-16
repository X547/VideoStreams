sources = [
	'ProducerApp.cpp',
	'VideoFileProducer.cpp',
]

VideoFileProducer = executable(
	'VideoFileProducer',
	sources,
	include_directories: [
		'../Library',
		'/boot/system/develop/headers/private/app',
		'/boot/system/develop/headers/private/shared'
	],
	dependencies: [
		compiler.find_library('be'),
		compiler.find_library('media'),
	],
	link_with: [
		libvideostreams
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
