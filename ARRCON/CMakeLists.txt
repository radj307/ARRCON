cmake_minimum_required (VERSION 3.13)

include(AutoVersion)
PARSE_VERSION_STRING("${CMAKE_SOURCE_DIR}" ARRCON_VERSION_MAJOR ARRCON_VERSION_MINOR ARRCON_VERSION_PATCH ARRCON_VERSION_EXTRA)
CREATE_VERSION_HEADER("ARRCON" ${ARRCON_VERSION_MAJOR} ${ARRCON_VERSION_MINOR} ${ARRCON_VERSION_PATCH})

CONCAT_VERSION_STRING(${IN_NAME}_VERSION "${ARRCON_VERSION_MAJOR}" "${ARRCON_VERSION_MINOR}" "${ARRCON_VERSION_PATCH}")

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
add_executable (ARRCON ${HEADERS} ${SRC} "ARRCON.rc")
target_include_directories(ARRCON PUBLIC "${CMAKE_SOURCE_DIR}/307lib/${CMAKE_BUILD_TYPE}/include")
target_link_directories(ARRCON PUBLIC "${CMAKE_SOURCE_DIR}/307lib/${CMAKE_BUILD_TYPE}/lib")

target_link_libraries(ARRCON PUBLIC shared strlib TermAPI filelib optlib)

install( # Installation target for the ARRCON executable.
	TARGETS ARRCON
	RUNTIME
	DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
)
