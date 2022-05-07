sources = [
	'ConsumerApp.cpp',
	'AccelerantConsumer.cpp',
]

AccelerantConsumer = executable(
	'AccelerantConsumer',
	sources,
	include_directories: [
		'/boot/system/develop/headers/private/app'
	],
	dependencies: [
		VideoStreams,
		compiler.find_library('be'),
		compiler.find_library('game')
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
