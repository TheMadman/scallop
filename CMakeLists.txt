cmake_minimum_required(VERSION 3.22.1)

project(ScallopShell)

find_package(Ceasoning REQUIRED)

add_subdirectory(src)

if(BUILD_TESTING)
	enable_testing()
	add_subdirectory(tests)
endif(BUILD_TESTING)
