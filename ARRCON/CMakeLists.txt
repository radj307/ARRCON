# ARRCON/ARRCON
cmake_minimum_required (VERSION 3.15)

if (MSVC)
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Create project
project("ARRCON" VERSION ${ARRCON_VERSION} LANGUAGES CXX)

CREATE_VERSION_HEADER("${CMAKE_CURRENT_BINARY_DIR}/version.h" "${ARRCON_VERSION_MAJOR}" "${ARRCON_VERSION_MINOR}" "${ARRCON_VERSION_PATCH}")
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

# Create resource files
include(ResourceMaker)
CREATE_VERSION_RESOURCE(
	"${CMAKE_CURRENT_BINARY_DIR}/ARRCON.rc"
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
if (WIN32)
	APPEND_ICON_RESOURCE("${CMAKE_CURRENT_BINARY_DIR}/ARRCON.rc" "${CMAKE_CURRENT_SOURCE_DIR}/ARRCON.ico")
endif()

# Create executable
add_executable (ARRCON "main.cpp" "${CMAKE_CURRENT_BINARY_DIR}/ARRCON.rc")
set_property(TARGET ARRCON PROPERTY CXX_STANDARD 20)
set_property(TARGET ARRCON PROPERTY CXX_STANDARD_REQUIRED ON)
if (MSVC)
	target_compile_options(ARRCON PUBLIC "/Zc:__cplusplus")
endif()

# Add headers
target_sources(ARRCON PUBLIC ${HEADERS})

# Check if user has 307lib as a submodule or as an installation, include necessary directories if using an installation
option(307lib_USE_INSTALLATION_NOT_COPY OFF "Use an installed version of the 307lib library, instead of the git submodule version. This is faster to compile.")
if (307lib_USE_INSTALLATION_NOT_COPY)
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
