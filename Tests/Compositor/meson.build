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
		dep_libvideostreams,
		dep_libbe,
		dep_libaccelerant,
		dep_libtest_shared,
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
