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
set(RC
	"ARRCON.rc"
)

# Add source to this project's executable.
add_executable (ARRCON ${HEADERS} ${SRC} ${RC})
#set_target_properties(ARRCON PROPERTIES LINKER_LANGUAGE CXX)
