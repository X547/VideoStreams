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
		dep_libvideostreams,
		dep_libbe,
		dep_libmedia,
		dep_libaccelerant,
		dep_libtest_shared,
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
