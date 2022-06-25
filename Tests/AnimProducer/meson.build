sources = [
	'ProducerApp.cpp',
	'AnimProducer.cpp',
]

AnimProducer = executable(
	'AnimProducer',
	sources,
	include_directories: [
		'/boot/system/develop/headers/private/app',
		'/boot/system/develop/headers/private/shared'
	],
	dependencies: [
		VideoStreams,
		compiler.find_library('be'),
		compiler.find_library('translation'),
	],
	gnu_symbol_visibility: 'hidden',
	install: true,
	install_dir: ''
)

install_data(
	[
		'../rsrc/comipo24bit/01.png',
		'../rsrc/comipo24bit/02.png',
		'../rsrc/comipo24bit/03.png',
		'../rsrc/comipo24bit/04.png',
		'../rsrc/comipo24bit/05.png',
		'../rsrc/comipo24bit/06.png',
		'../rsrc/comipo24bit/07.png',
		'../rsrc/comipo24bit/08.png',
		'../rsrc/comipo24bit/09.png',
		'../rsrc/comipo24bit/10.png',
		'../rsrc/comipo24bit/11.png',
		'../rsrc/comipo24bit/12.png',
		'../rsrc/comipo24bit/13.png',
		'../rsrc/comipo24bit/14.png',
		'../rsrc/comipo24bit/15.png',
		'../rsrc/comipo24bit/16.png',
		'../rsrc/comipo24bit/17.png',
		'../rsrc/comipo24bit/18.png',
		'../rsrc/comipo24bit/19.png',
		'../rsrc/comipo24bit/20.png',
		'../rsrc/comipo24bit/21.png',
		'../rsrc/comipo24bit/22.png',
		'../rsrc/comipo24bit/23.png',
		'../rsrc/comipo24bit/24.png',
		'../rsrc/comipo24bit/25.png',
	],
	install_dir: 'rsrc/comipo24bit',
)
