# ARRCON/UnitTests
cmake_minimum_required(VERSION 3.20)

set(CATCH_INSTALL_DOCS FALSE CACHE BOOL "" FORCE)

add_subdirectory("catch2")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/catch2/extras")

file(GLOB HEADERS
	RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
	CONFIGURE_DEPENDS
	"*.h*"
)
file(GLOB SRCS
	RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
	CONFIGURE_DEPENDS
	"*.c*"
)

add_executable(UnitTests ${SRCS} "cpp.hint")

set_property(TARGET UnitTests PROPERTY CXX_STANDARD 20)
set_property(TARGET UnitTests PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET UnitTests PROPERTY POSITION_INDEPENDENT_CODE ON)
if (MSVC)
	target_compile_options(UnitTests PRIVATE "/Zc:__cplusplus")
endif()

target_include_directories(UnitTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/catch2/src")

target_sources(UnitTests PRIVATE ${HEADERS})

target_link_libraries(UnitTests PRIVATE shared strlib TermAPI filelib optlib libunistd Catch2::Catch2WithMain)

include(CTest)
include(Catch)

CATCH_DISCOVER_TESTS(UnitTests)
