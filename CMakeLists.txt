# ARRCON
cmake_minimum_required (VERSION 3.19)

# Add 307modules to cmake modules path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/307lib/307modules")

include(AutoVersion2)

set(ENV{ARRCON_VERSION} "0.0.0")
GET_VERSION_2("ARRCON" "${CMAKE_CURRENT_SOURCE_DIR}" MAJOR MINOR PATCH EXTRA)

project("ARRCON" VERSION ${ARRCON_VERSION} LANGUAGES CXX)


option(307lib_USE_INSTALLATION "When true, uses an installed version of the 307lib library instead of a git submodule." OFF)
if (307lib_USE_INSTALLATION)
	set(307lib_PATH "${CMAKE_CURRENT_SOURCE_DIR}/307lib" CACHE STRING "The path to the root 307lib installation directory. (Contains <Debug|Release|MinSizeRel|RelWithDebInfo> depending on your desired build type)")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/307lib/CMakeLists.txt")
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


option(ARRCON_ENABLE_TESTING "Enable the ARRCON unit testing subproject" ON)

if (ARRCON_ENABLE_TESTING AND WIN32) # Todo: Figure out why POSIX can't compile unit tests
	add_subdirectory("UnitTests")
endif()
