subdir('Vulkan')

libtest_shared = static_library(
	'test_shared',
	[
		'FindConsumer.cpp',
	],
	include_directories: [
		'/boot/system/develop/headers/private/shared'
	],
	dependencies: [
		dep_libvideostreams,
		dep_libbe,
		dep_libaccelerant,
	],
	gnu_symbol_visibility: 'hidden',
)

dep_libtest_shared = declare_dependency(
	link_with: libtest_shared,
	dependencies: [
		dep_libaccelerant,
	],
	include_directories: [
		'.'
	],
)
