# ARRCON
cmake_minimum_required (VERSION 3.19)

# Set C++ Language Version to 20, or latest on MSVC to fix an MSVC/Windows-specific bug
if(MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.29.30129 AND CMAKE_VERSION VERSION_GREATER 3.20.3)
	set(CMAKE_CXX_STANDARD 23) # MSVC
else()
	set(CMAKE_CXX_STANDARD 20)
endif()
# Require C++20 as concepts & other C++20 features are used.
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project ("ARRCON") # Project

if (MSVC) # MSVC-specific requirements
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
	string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus")
endif()

add_subdirectory ("ARRCON")		# ARRCON Project Executable

if (WIN32)
	add_subdirectory(libunistd) # replace the unistd library with "libunistd" on windows
	target_include_directories(libunistd PUBLIC "libunistd" "libunistd/unistd")
	target_link_libraries(ARRCON PUBLIC libunistd)
endif()
