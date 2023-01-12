sources = [
	'ConsumerApp.cpp',
]

WindowConsumer = executable(
	'WindowConsumer',
	sources,
	include_directories: [
		'/boot/system/develop/headers/private/app'
	],
	dependencies: [
		dep_libvideostreams,
		dep_libbe,
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
