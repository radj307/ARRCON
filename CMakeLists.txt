# ARRCON
cmake_minimum_required (VERSION 3.19)

set(CMAKE_CXX_STANDARD 20)
if (MSVC) # MSVC-specific requirements
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
	string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus")
endif()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

add_subdirectory ("ARRCON")		# ARRCON Project Executable

if (WIN32)
	add_subdirectory(libunistd) # replace the unistd library with "libunistd" on windows
	target_include_directories(libunistd PUBLIC "libunistd" "libunistd/unistd")
	target_link_libraries(ARRCON PUBLIC libunistd)
endif()
