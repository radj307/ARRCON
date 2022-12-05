# ARRCON/ARRCON
cmake_minimum_required (VERSION 3.22)

file(GLOB_RECURSE HEADERS
	RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
	CONFIGURE_DEPENDS
	"*.h*"
)
file(GLOB_RECURSE SRCS
	RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
	CONFIGURE_DEPENDS
	"*.c*"
)

include(ResourceMaker)

if (WIN32)
	file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/rc")

	MAKE_STRINGRC_VERSIONINFO(
		_arrcon_stringrc_versioninfo
		"${ARRCON_VERSION}"
		"Copyright © 2022 by radj307"
		"radj307"
		"ARRCON"
		"Commandline client for communicating with servers using the Source RCON Protocol."
	)
	MAKE_STRINGRC_ICON(
		_arrcon_stringrc_icon
		"${CMAKE_CURRENT_SOURCE_DIR}/ARRCON.ico"
	)
	
	set(_arrcon_rc "${CMAKE_CURRENT_BINARY_DIR}/rc/ARRCON.rc")
	MAKE_RESOURCE("${_arrcon_rc}" "${_arrcon_stringrc_versioninfo}" "${_arrcon_stringrc_icon}")
endif()

MAKE_VERSION_HEADER("${CMAKE_CURRENT_BINARY_DIR}/rc/version.h" ARRCON "${ARRCON_VERSION_EXTENDED}")
include(CopyrightMaker)
MAKE_COPYRIGHT_HEADER("${CMAKE_CURRENT_BINARY_DIR}/rc/copyright.h" ARRCON 2022 radj307)

add_executable(ARRCON "${SRCS}" "${_arrcon_rc}" "${CMAKE_CURRENT_BINARY_DIR}/rc/version.h" "${CMAKE_CURRENT_BINARY_DIR}/rc/copyright.h")

set_property(TARGET ARRCON PROPERTY CXX_STANDARD 20)
set_property(TARGET ARRCON PROPERTY CXX_STANDARD_REQUIRED ON)

if (MSVC)
	set_property(TARGET ARRCON PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS ON)
	target_compile_options(ARRCON PRIVATE "${307lib_compiler_commandline}")
endif()

target_include_directories(ARRCON PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/rc")

target_sources(ARRCON PRIVATE "${HEADERS}")

# Setup libunistd
if (WIN32)
	include(FetchContent)
	set(libunistd_name "libunistd")
	FetchContent_Declare(
		"${libunistd_name}"
		GIT_REPOSITORY	https://github.com/robinrowe/libunistd.git
		GIT_TAG			origin/master
	)
	FetchContent_MakeAvailable("${libunistd_name}")
endif()

if (APPLE)
	set(_arrcon_linker_flags "-static -static-libgcc -static-libstdc++")
endif()

if (NOT DEFINED _arrcon_linker_flags)
	set(_arrcon_linker_flags PRIVATE)
endif()

target_link_libraries(ARRCON ${_arrcon_linker_flags} TermAPI filelib "${libunistd_name}")

include(PackageInstaller)

INSTALL_EXECUTABLE(ARRCON "${CMAKE_INSTALL_PREFIX}/bin")
