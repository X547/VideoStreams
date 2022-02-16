sources = [
	'CompositeApp.cpp',
]

Compositor = executable(
	'Compositor',
	sources,
	include_directories: [
		'../Library',
		'/boot/system/develop/headers/private/app',
		'/boot/system/develop/headers/private/shared'
	],
	dependencies: [
		compiler.find_library('be'),
	],
	link_with: [
		libvideostreams
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
