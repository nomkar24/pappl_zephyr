if(CONFIG_PAPPL)

cmake_minimum_required(VERSION 3.12...3.31)
project(pappl_zephyr)

FILE(GLOB PAPPL_SRCS *.c)

zephyr_library_named(PAPPL)

zephyr_include_directories(.)

zephyr_library_sources( ${PAPPL_SRCS} )

zephyr_library_compile_options(-D_THREAD_SAFE -D_REENTRANT -D_FORTIFY_SOURCE=2 -D__USE_MISC -D_GNU_SOURCE -D_TIME_BITS=64 -D_FILE_OFFSET_BITS=64 -g -Os -Wall -Wunused -Wno-char-subscripts -Wno-deprecated-declarations -Wno-format-truncation -Wno-format-y2k -Wno-switch -Wno-unused-result)

zephyr_library_compile_options(-U_POSIX_C_SOURCE -D_POSIX_C_SOURCE=200809L)

endif(CONFIG_PAPPL)
