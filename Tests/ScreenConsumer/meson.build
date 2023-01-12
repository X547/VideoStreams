sources = [
	'ConsumerApp.cpp',
]

ScreenConsumer = executable(
	'ScreenConsumer',
	sources,
	include_directories: [
		'/boot/system/develop/headers/private/app'
	],
	dependencies: [
		dep_libvideostreams,
		dep_libbe,
		dep_libgame
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
