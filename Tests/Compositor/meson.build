sources = [
	'CompositeApp.cpp',
]

Compositor = executable(
	'Compositor',
	sources,
	include_directories: [
		'/boot/system/develop/headers/private/app',
		'/boot/system/develop/headers/private/shared'
	],
	dependencies: [
		VideoStreams,
		compiler.find_library('be'),
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
