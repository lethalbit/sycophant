# SPDX-License-Identifier: BSD-3-Clause
project(
	'sycophant',
	'cpp',
	default_options: [
		'buildtype=release',
		'cpp_std=c++17',
		'warning_level=3',
		'b_ndebug=if-release',
		'b_lto=true',
		'strip=false'
	],
	license: 'BSD-3-Clause',
	version: '0.0.1',
	meson_version: '>= 1.0.0',
	subproject_dir: 'deps'
)

cxx = meson.get_compiler('cpp')
if get_option('cpp_std') not in ['c++17', 'c++20', 'c++23', 'gnu++17', 'gnu++20', 'gnu++23']
	error('Unsupported C++ Version @0@, must be c++17/gnu++17 or newer'.format(get_option('cpp_std')))
endif

extended_warnings = [
	'-Wdouble-promotion',
	'-Wformat=2',
	'-Wformat-overflow=2',
	'-Wformat-signedness',
	'-Wformat-truncation',
	'-Wnull-dereference',
	'-Wmissing-attributes',
	'-Wmissing-braces',
	'-Wsequence-point',
	'-Wreturn-type',
	'-Wunused',
	'-Wunused-local-typedefs',
	'-Wunused-const-variable=2',
	'-Wmaybe-uninitialized',
	'-Wunknown-pragmas',
	'-Wstrict-aliasing',
	'-Wstrict-overflow=3',
	'-Wstring-compare',
	'-Wstringop-overflow',
	'-Warith-conversion',
	'-Wvla-parameter',
	'-Wduplicated-branches',
	'-Wshadow=local',
	'-Wunsafe-loop-optimizations',
	'-Wbad-function-cast',
	'-Wcast-qual',
	'-Wcast-align=strict',
	'-Wcast-function-type',
	'-Wconversion',
	'-Wdangling-else',
	'-Wsign-conversion',
	'-Wfloat-conversion',
	'-Wredundant-decls',
	'-Winline',
	'-Wvla',
	'-Wstack-protector',
	'-Wunsuffixed-float-constant',
	'-Wimplicit-fallthrough',
]

add_project_arguments(
	cxx.get_supported_arguments(extended_warnings),
	language: 'cpp'
)

is_clang         = (cxx.get_id() == 'clang')
is_gcc           = (cxx.get_id() == 'gcc')
compiler_version = cxx.version()

py = import('python')
py = py.find_installation(
	'python3',
	required: true,
	modules: []
)

pybind11 = dependency('pybind11',  required: false, version: '>=2.10.0' )

if not pybind11.found()
	message('Did not find local pybind11 install, bundling')
	pybind11_wrap = subproject('pybind11', default_options: [])
	pybind11 = pybind11_wrap.get_variable('pybind11_dep')
endif

subdir('src')
