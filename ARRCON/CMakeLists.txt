cmake_minimum_required (VERSION 3.13)

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
