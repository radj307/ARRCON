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

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/rc")
if (WIN32)
	include(ResourceMaker)

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
	
	MAKE_RESOURCE("${CMAKE_CURRENT_BINARY_DIR}/rc/ARRCON.rc" "${_arrcon_stringrc_versioninfo}" "${_arrcon_stringrc_icon}")
endif()

MAKE_VERSION_HEADER("${CMAKE_CURRENT_BINARY_DIR}/rc/version.h" ARRCON "${ARRCON_VERSION_EXTENDED}")
include(CopyrightMaker)
MAKE_COPYRIGHT_HEADER("${CMAKE_CURRENT_BINARY_DIR}/rc/copyright.h" ARRCON 2022 radj307)

file(GLOB RESOURCES
	CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_BINARY_DIR}/rc/*"
)

add_executable(ARRCON "${SRCS}" "${RESOURCES}")

set_property(TARGET ARRCON PROPERTY CXX_STANDARD 20)
set_property(TARGET ARRCON PROPERTY CXX_STANDARD_REQUIRED ON)

if (MSVC)
	target_compile_options(ARRCON PRIVATE "${307lib_compiler_commandline}")
endif()

target_include_directories(ARRCON PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/rc")

target_sources(ARRCON PRIVATE "${HEADERS}")

# Setup Boost
include(FetchContent)
FetchContent_Declare(
	Boost
	GIT_REPOSITORY	https://github.com/boostorg/boost.git
	GIT_TAG			boost-1.84.0
)
#set(BOOST_INCLUDE_LIBRARIES asio stacktrace)
FetchContent_MakeAvailable(Boost)

target_link_libraries(ARRCON PRIVATE TermAPI filelib Boost::asio)

# Link Boost::Stacktrace
if(ARRCON_DISABLE_STACKTRACE)
	# Disable stacktrace
	target_link_libraries(ARRCON PRIVATE boost_stacktrace_noop)	
elseif(MSVC)
	# Use COM stacktrace
	target_link_libraries(ARRCON PRIVATE boost_stacktrace_windbg ole32 dbgeng)
else()
	# Use POSIX backtrace
	target_link_libraries(ARRCON PRIVATE boost_stacktrace_backtrace dl backtrace)
endif()

include(PackageInstaller)

INSTALL_EXECUTABLE(ARRCON "${CMAKE_INSTALL_PREFIX}/bin")
