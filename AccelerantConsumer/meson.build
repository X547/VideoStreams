sources = [
	'ConsumerApp.cpp',
	'AccelerantConsumer.cpp',
]

AccelerantConsumer = executable(
	'AccelerantConsumer',
	sources,
	include_directories: [
		'../Library',
		'/boot/system/develop/headers/private/app'
	],
	dependencies: [
		compiler.find_library('be'),
		compiler.find_library('game')
	],
	link_with: [
		libvideostreams
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
