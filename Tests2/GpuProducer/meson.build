project('GpuProducer', 'cpp',
	version : '0.1',
	default_options : ['warning_level=3', 'cpp_std=c++2a']
)

add_project_arguments('-Wno-missing-field-initializers', language: 'cpp')

compiler = meson.get_compiler('cpp')


sources = [
	'ProducerApp.cpp',
	'TestProducer.cpp',
]

TestProducer = executable(
	'TestProducer',
	sources,
	include_directories: [
		'/boot/system/develop/headers/private/app',
		'/boot/system/develop/headers/private/shared'
	],
	dependencies: [
		dependency('libvideostreams'),
		dependency('libdrm'),
		dependency('libdrm_amdgpu'),
		compiler.find_library('be'),
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
