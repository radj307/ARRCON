# ARRCON/ARRCON
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

string(TIMESTAMP _current_year "%Y")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/rc")
if (WIN32)
	include(ResourceMaker)

	MAKE_STRINGRC_VERSIONINFO(
		_arrcon_stringrc_versioninfo
		"${ARRCON_VERSION}"
		"Copyright © ${_current_year} by radj307"
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
MAKE_COPYRIGHT_HEADER("${CMAKE_CURRENT_BINARY_DIR}/rc/copyright.h" ARRCON ${_current_year} radj307)

file(GLOB RESOURCES
	CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_BINARY_DIR}/rc/*"
)

include_directories("/opt/local/include")

add_executable(ARRCON "${SRCS}" "${RESOURCES}")

set_property(TARGET ARRCON PROPERTY CXX_STANDARD 20)
set_property(TARGET ARRCON PROPERTY CXX_STANDARD_REQUIRED ON)

if (MSVC)
	target_compile_options(ARRCON PRIVATE "${307lib_compiler_commandline}")
endif()

target_include_directories(ARRCON PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/rc")

target_sources(ARRCON PRIVATE "${HEADERS}")

## Setup Boost:
# Try to find an existing Boost 1.84.0 package
find_package(Boost 1.84.0 COMPONENTS asio)
# Fallback to FetchContent if not found
if (NOT Boost_FOUND)
	message(STATUS "Downloading Boost 1.84.0 via FetchContent")

	include(FetchContent)
	FetchContent_Declare(
		Boost
		GIT_REPOSITORY	https://github.com/boostorg/boost.git
		GIT_TAG			boost-1.84.0
	)
	FetchContent_MakeAvailable(Boost)
endif()

target_link_libraries(ARRCON PRIVATE
	TermAPI
	filelib
	Boost::asio
)
