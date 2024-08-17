project('quill', 'cpp', version : '7.0.0', default_options : ['warning_level=3', 'cpp_std=c++17'])

inc_dirs = include_directories('include')

interface_compile_args = meson.get_compiler('cpp').get_supported_arguments('-Wno-gnu-zero-variadic-macro-arguments')

quill_dep = declare_dependency(include_directories : inc_dirs,
                               dependencies : [dependency('threads')],
                               compile_args : interface_compile_args)

install_subdir('include/quill', install_dir : get_option('includedir'), install_tag : 'devel')

pkg_mod = import('pkgconfig')
pkg_mod.generate(
    name : 'quill',
    filebase : 'quill',
    description : 'Asynchronous Low Latency C++ Logging Library',
    subdirs : 'quill'
)
