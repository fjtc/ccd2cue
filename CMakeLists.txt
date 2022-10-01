cmake_minimum_required(VERSION 3.16)
project(ccd2cue VERSION 0.5)

find_package(Iconv REQUIRED)
find_package(Gettext REQUIRED)
find_package(Intl REQUIRED)

configure_file(src/config.h.cmake.in src/config.h)

add_executable(ccd2cue
	src/config.h
	src/file.h
	src/cdt.h
	src/file.c
	src/memory.h
	src/array.c
	src/i18n.h
	src/errors.c
	src/cdt.c
	src/convert.c
	src/crc.c
	src/ccd.h
	src/cue.h
	src/io.c
	src/array.h
	src/crc.h
	src/io.h
	src/cue.c
	src/errors.h
	src/convert.h
	src/memory.c
	src/ccd.c
	src/ccd2cue.c)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)
target_include_directories(ccd2cue PUBLIC "src")
