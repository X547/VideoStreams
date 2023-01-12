sources = [
	'ProducerApp.cpp',
	'TestProducer.cpp',
	'AnimProducer.cpp',
]

TestProducer = executable(
	'TestProducer',
	sources,
	include_directories: [
		'/boot/system/develop/headers/private/app',
		'/boot/system/develop/headers/private/shared'
	],
	dependencies: [
		dep_libvideostreams,
		dep_libbe,
		dep_libtranslation,
		dep_libtest_shared,
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
