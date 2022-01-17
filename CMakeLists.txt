# ARRCON
cmake_minimum_required (VERSION 3.19)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/307lib/307modules")

set(CMAKE_CXX_STANDARD 20)
if (MSVC) # MSVC-specific requirements
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
	string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus")
endif()

include(AutoVersion)
GET_VERSION("${CMAKE_SOURCE_DIR}" ARRCON_VERSION ARRCON_VERSION_MAJOR ARRCON_VERSION_MINOR ARRCON_VERSION_PATCH ARRCON_VERSION_EXTRA)
CREATE_VERSION_HEADER(ARRCON ${ARRCON_VERSION_MAJOR} ${ARRCON_VERSION_MINOR} ${ARRCON_VERSION_PATCH})

project("ARRCON" VERSION ${ARRCON_VERSION} LANGUAGES CXX)

option(307lib_USE_INSTALLATION "When true, uses an installed version of the 307lib library instead of a git submodule." OFF)
if (307lib_USE_INSTALLATION)
	set(307lib_PATH "${CMAKE_SOURCE_DIR}/307lib" CACHE STRING "The path to the root 307lib installation directory. (Contains <Debug|Release|MinSizeRel|RelWithDebInfo> depending on your desired build type)")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/307lib/CMakeLists.txt")
	add_subdirectory("307lib")
else()
	message(FATAL_ERROR "Cannot find a valid installation of 307lib!")
endif()

add_subdirectory ("ARRCON")		# ARRCON Project Executable

if (WIN32)
	add_subdirectory(libunistd) # replace the unistd library with "libunistd" on windows
	target_include_directories(libunistd PUBLIC "libunistd" "libunistd/unistd")
	target_link_libraries(ARRCON PUBLIC libunistd)
endif()
