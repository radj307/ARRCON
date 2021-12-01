# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.22)

if(MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.29.30129 AND CMAKE_VERSION VERSION_GREATER 3.20.3)
	set(CMAKE_CXX_STANDARD 23)
else()
	set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)


project ("ARRCON")

if (MSVC) # Make MSVC use standard __cplusplus macro values
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
	string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus")

	add_subdirectory(libunistd)
	target_include_directories(libunistd PUBLIC "libunistd" "libunistd/unistd")
endif()


add_subdirectory(sharedlib)		# Shared Library
add_subdirectory(TermAPI)		# TermAPI library
add_subdirectory(optlib)		# cli-opt library
add_subdirectory(FileLib)		# cpp-file-parser library

add_subdirectory ("ARRCON")						# ARRCON Project Executable

if (MSVC)
	target_link_libraries(ARRCON PUBLIC libunistd)
endif()

target_link_libraries(ARRCON PUBLIC sharedlib TermAPI optlib FileLib)