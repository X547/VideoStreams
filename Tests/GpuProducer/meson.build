dep_libdrm = dependency('libdrm')
dep_libdrm_amdgpu = dependency('libdrm_amdgpu')


TestProducer = executable(
	'GpuProducer',
	[
		'ProducerApp.cpp',
		'GpuProducer.cpp',
	],
	include_directories: [
		'/boot/system/develop/headers/private/app',
		'/boot/system/develop/headers/private/shared'
	],
	dependencies: [
		dep_libbe,
		dep_libvideostreams,
		dep_libaccelerant,
		dep_libdrm,
		dep_libdrm_amdgpu,
		dep_libtest_shared,
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
