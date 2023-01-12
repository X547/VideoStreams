WindowApp = executable(
	'WindowApp',
	[
		'ConsumerApp.cpp',
		'TestProducer.cpp',
		'TestFilter.cpp',
	],
	include_directories: [
		'/boot/system/develop/headers/private/shared'
	],
	dependencies: [
		dep_libvideostreams,
		dep_libbe,
		dep_libtest_shared,
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
