project('MinApp', 'cpp',
	version : '0.1',
	default_options : ['warning_level=3', 'cpp_std=c++2a']
)

add_project_arguments('-Wno-missing-field-initializers', language: 'cpp')

compiler = meson.get_compiler('cpp')

subdir('Library')
subdir('TestProducer')
subdir('VideoFileProducer')
subdir('AnimProducer')
subdir('WindowConsumer')
subdir('ScreenConsumer')
subdir('AccelerantConsumer')
subdir('Compositor')
