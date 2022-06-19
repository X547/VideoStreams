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
		'../rsrc/comipo24bit/comipo24bit01.png',
		'../rsrc/comipo24bit/comipo24bit02.png',
		'../rsrc/comipo24bit/comipo24bit03.png',
		'../rsrc/comipo24bit/comipo24bit04.png',
		'../rsrc/comipo24bit/comipo24bit05.png',
		'../rsrc/comipo24bit/comipo24bit06.png',
		'../rsrc/comipo24bit/comipo24bit07.png',
		'../rsrc/comipo24bit/comipo24bit08.png',
		'../rsrc/comipo24bit/comipo24bit09.png',
		'../rsrc/comipo24bit/comipo24bit10.png',
		'../rsrc/comipo24bit/comipo24bit11.png',
		'../rsrc/comipo24bit/comipo24bit12.png',
		'../rsrc/comipo24bit/comipo24bit13.png',
		'../rsrc/comipo24bit/comipo24bit14.png',
		'../rsrc/comipo24bit/comipo24bit15.png',
		'../rsrc/comipo24bit/comipo24bit16.png',
		'../rsrc/comipo24bit/comipo24bit17.png',
		'../rsrc/comipo24bit/comipo24bit18.png',
		'../rsrc/comipo24bit/comipo24bit19.png',
		'../rsrc/comipo24bit/comipo24bit20.png',
		'../rsrc/comipo24bit/comipo24bit21.png',
		'../rsrc/comipo24bit/comipo24bit22.png',
		'../rsrc/comipo24bit/comipo24bit23.png',
		'../rsrc/comipo24bit/comipo24bit24.png',
		'../rsrc/comipo24bit/comipo24bit25.png',
	],
	install_dir: 'rsrc/comipo24bit',
)
