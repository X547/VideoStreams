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
		VideoStreams,
		compiler.find_library('be')
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
