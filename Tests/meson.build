project('VideoStreamsTests', 'cpp',
	version : '0.1',
	default_options : ['warning_level=3', 'cpp_std=c++2a']
)

add_project_arguments('-Wno-missing-field-initializers', language: 'cpp')

compiler = meson.get_compiler('cpp')

VideoStreamsProj = subproject('VideoStreams')

dep_libbe = compiler.find_library('be')
dep_libtranslation = compiler.find_library('translation')
dep_libmedia = compiler.find_library('media')
dep_libgame = compiler.find_library('game')
dep_libvideostreams = VideoStreamsProj.get_variable('VideoStreams')
dep_libvideostreams_vulkan = VideoStreamsProj.get_variable('VideoStreamsVulkan')
dep_libaccelerant = dependency('libaccelerant')
dep_libvulkan = dependency('vulkan')

subdir('Shared')

subdir('TestProducer')
subdir('TestProducerLoop')
subdir('VideoFileProducer')
subdir('AnimProducer')
subdir('WindowConsumer')
subdir('WindowConsumerGpu')
subdir('ScreenConsumer')
subdir('AccelerantConsumer')
subdir('Compositor')
subdir('WindowApp')
subdir('GpuProducer')
subdir('VulkanBind')
subdir('VulkanGpuProducer')
