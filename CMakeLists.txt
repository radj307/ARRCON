# ARRCON/
cmake_minimum_required (VERSION 3.22)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/307lib/307modules")

include(VersionTag)

set(ENV{ARRCON_VERSION} "0.0.0")
GET_VERSION_TAG("${CMAKE_CURRENT_SOURCE_DIR}" "ARRCON")

project("ARRCON" VERSION ${ARRCON_VERSION} LANGUAGES CXX)

add_subdirectory("307lib")
add_subdirectory("ARRCON")
