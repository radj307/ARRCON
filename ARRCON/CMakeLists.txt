cmake_minimum_required (VERSION 3.13)

# Add source to this project's executable.
add_executable (ARRCON "main.cpp" "ARRCON.rc")
#set_target_properties(ARRCON PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(ARRCON INTERFACE .)
