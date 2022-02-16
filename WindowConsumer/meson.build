sources = [
	'ConsumerApp.cpp',
	'ConsumerView.cpp'
]

WindowConsumer = executable(
	'WindowConsumer',
	sources,
	include_directories: [
		'../Library',
		'/boot/system/develop/headers/private/app'
	],
	dependencies: [
		compiler.find_library('be')
	],
	link_with: [
		libvideostreams
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)
