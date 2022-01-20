# ARRCON/ARRCON
cmake_minimum_required (VERSION 3.15)

if (MSVC)
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Create project
project("ARRCON" VERSION ${ARRCON_VERSION} LANGUAGES CXX)

include(ResourceMaker)

file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/version.h")
CREATE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/version.h"
	"#pragma once\\n"
	"#define ARRCON_VERSION ${ARRCON_VERSION}\\n"
	"#define ARRCON_VERSION_MAJOR ${ARRCON_VERSION_MAJOR}\\n"
	"#define ARRCON_VERSION_MINOR ${ARRCON_VERSION_MINOR}\\n"
	"#define ARRCON_VERSION_PATCH ${ARRCON_VERSION_PATCH}\\n"
)

set(HEADERS
	"globals.h"
	"config.hpp"
	"Help.hpp"
	"packet.hpp"
	"net.hpp"
	"rcon.hpp"
	"mode.hpp"
	"${CMAKE_CURRENT_BINARY_DIR}/version.h"
)

option(ARRCON_ENABLE_ADDITIONAL_RESOURCES "Enable resource file generation" OFF)

# Create resource files
if (WIN32)
	set(ARRCON_RESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ARRCON.rc")
	file(REMOVE "${ARRCON_RESOURCE_FILE}")
	file(TOUCH "${ARRCON_RESOURCE_FILE}")

	CREATE_VERSION_RESOURCE(
		"${ARRCON_RESOURCE_FILE}"
		"${ARRCON_VERSION_MAJOR}"
		"${ARRCON_VERSION_MINOR}"
		"${ARRCON_VERSION_PATCH}"
		"radj307"
		"Commandline client for communicating with servers using the Source RCON Protocol."
		"ARRCON"
		"GPLv3"
		"ARRCON"
		"ARRCON"
	)
	APPEND_ICON_RESOURCE("${ARRCON_RESOURCE_FILE}" "${CMAKE_CURRENT_SOURCE_DIR}/ARRCON.ico")
endif()

# Create executable
add_executable (ARRCON "main.cpp" "${ARRCON_RESOURCE_FILE}")
set_property(TARGET ARRCON PROPERTY CXX_STANDARD 20)
set_property(TARGET ARRCON PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ARRCON PROPERTY POSITION_INDEPENDENT_CODE ON)
if (MSVC)
	target_compile_options(ARRCON PUBLIC "/Zc:__cplusplus")
endif()

# Add headers
target_sources(ARRCON PUBLIC ${HEADERS})

# Check if user has 307lib as a submodule or as an installation, include necessary directories if using an installation
option(ARRCON_USE_307lib_INSTALLATION OFF "Use an installed version of the 307lib library, instead of the git submodule version. This is faster to compile.")
if (ARRCON_USE_307lib_INSTALLATION)
	target_include_directories(ARRCON PUBLIC "${307lib_PATH}/${CMAKE_BUILD_TYPE}/include")
	target_link_directories(ARRCON PUBLIC "${307lib_PATH}/${CMAKE_BUILD_TYPE}/lib")
endif()

# Link dependencies
target_link_libraries(ARRCON PUBLIC shared strlib TermAPI filelib optlib)

# Create installation target
install(
	TARGETS ARRCON
	RUNTIME
	DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
)
