# ARRCON/ARRCON
cmake_minimum_required (VERSION 3.13)

include(AutoVersion)
GET_VERSION("${CMAKE_SOURCE_DIR}" ARRCON_VERSION_MAJOR ARRCON_VERSION_MINOR ARRCON_VERSION_PATCH ARRCON_VERSION_EXTRA)
MAKE_VERSION(ARRCON_VERSION "${ARRCON_VERSION_MAJOR}" "${ARRCON_VERSION_MINOR}" "${ARRCON_VERSION_PATCH}")
CREATE_VERSION_HEADER(ARRCON ${ARRCON_VERSION_MAJOR} ${ARRCON_VERSION_MINOR} ${ARRCON_VERSION_PATCH})

project("ARRCON" VERSION ${ARRCON_VERSION} LANGUAGES CXX)

set(HEADERS
	"globals.h"
	"config.hpp"
	"Help.hpp"
	"packet.hpp"
	"net.hpp"
	"rcon.hpp"
	"mode.hpp"
)
set(SRC
	"main.cpp"
)

# Add source to this project's executable.
add_executable (ARRCON ${SRC} "ARRCON.rc")
target_sources(ARRCON PUBLIC ${HEADERS})
if (307lib_USE_INSTALLATION)
	target_include_directories(ARRCON PUBLIC "${307lib_PATH}/${CMAKE_BUILD_TYPE}/include")
	target_link_directories(ARRCON PUBLIC "${307lib_PATH}/${CMAKE_BUILD_TYPE}/lib")
endif()

target_link_libraries(ARRCON PUBLIC shared strlib TermAPI filelib optlib)

install( # Installation target for the ARRCON executable.
	TARGETS ARRCON
	RUNTIME
	DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
)
