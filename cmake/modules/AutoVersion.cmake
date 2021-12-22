# 307lib/cmake/modules/AutoVersion.cmake
# Contains functions for automatically setting package versions from git tags
cmake_minimum_required(VERSION 3.19)

set(AUTOVERSION_FIND_REGEX "^[vV]*([0-9]+)\.([0-9]+)\.([0-9]+)(.+)" CACHE STRING "Used by the AutoVersion PARSE_VERSION_STRING function to parse a git tag into a library version, by using capture groups. (1: Major Version ; 2: Minor Version ; 3: Patch Version ; 4: SHA1). All groups except for group 1 should be optional." FORCE)

#### PARSE_VERSION_STRING ####
# @brief				Retrieve and parse the result of calling `git describe --tags --dirty=-d`
# @param _out_major		Variable name to store the major version number
# @param _out_minor		Variable name to store the minor version number
# @param _out_patch		Variable name to store the patch version number
# @param _out_extra		Variable name to store extra characters in the version number
function(parse_version_string _working_dir _out_major _out_minor _out_patch _out_extra)
	# set the "VERSION_TAG" cache variable to the result of `git describe ${AUTOVERSION_GIT_DESCRIBE_ARGS}` in CMAKE_SOURCE_DIR
	execute_process(
		COMMAND 
			"git" 
			"describe" 
			"--tags"
		WORKING_DIRECTORY ${_working_dir}
		OUTPUT_VARIABLE VERSION_TAG
	)

	if (VERSION_TAG STREQUAL "") # Don't use ${} expansion here, if statements work without them and this may cause a comparison failure
		message(STATUS "No git tags found, skipping AutoVersioning.")
		return()
	endif()

	# Parse the version string using the provided regular expression
	string(REGEX REPLACE ${AUTOVERSION_FIND_REGEX} "\\1" _MAJOR ${VERSION_TAG}) # get Major
	string(REGEX REPLACE ${AUTOVERSION_FIND_REGEX} "\\2" _MINOR ${VERSION_TAG}) # get Minor
	string(REGEX REPLACE ${AUTOVERSION_FIND_REGEX} "\\3" _PATCH ${VERSION_TAG}) # get Patch
	string(REGEX REPLACE ${AUTOVERSION_FIND_REGEX} "\\4" _EXTRA ${VERSION_TAG}) # get Extra

	# Print a status message showing the parsed values
	message(STATUS "Successfully parsed version number from git tags.")
	message(STATUS "  MAJOR: ${_MAJOR}")
	message(STATUS "  MAJOR: ${_MINOR}")
	message(STATUS "  MAJOR: ${_PATCH}")
	message(STATUS "  EXTRA: ${_EXTRA}")

	# Set the provided output variable names to the parsed version numbers
	set(${_out_major} ${_MAJOR} CACHE STRING "(SEMVER) Major Version portion of the current git tag" FORCE)
	set(${_out_minor} ${_MINOR} CACHE STRING "(SEMVER) Minor Version portion of the current git tag" FORCE)
	set(${_out_patch} ${_PATCH} CACHE STRING "(SEMVER) Patch Version portion of the current git tag" FORCE)
	set(${_out_extra} ${_EXTRA} CACHE STRING "(SEMVER) Everything that appears after the first 3 (period-separated) digits in the current git tag." FORCE)

	# Cleanup
	unset(_MAJOR CACHE)
	unset(_MINOR CACHE)
	unset(_PATCH CACHE)
	unset(_EXTRA CACHE)
endfunction()

#### CONCAT_VERSION_STRING ####
# @brief			Concatenate version strings into a single version string in the format "${MAJOR}.${MINOR}.${PATCH}${EXTRA}"
# @param _out_ver	Name of the variable to use for output.
# @param _major		Major version number
# @param _minor		Minor version number
# @param _patch		Patch version number
# @param _extra		Extra portions of the version string.
function(concat_version_string _out_ver _major _minor _patch)
	set(${_out_ver} "${_major}.${_minor}.${_patch}" CACHE STRING "Full version string." FORCE)
endfunction()

#### CREATE_VERSION_HEADER ####
# @brief			Creates a copy of the version.h.in header in the caller's source directory.
# @param _name		The name of the library
# @param _major		Major version number
# @param _minor		Minor version number
# @param _patch		Patch version number
function(create_version_header _name _major _minor _patch)
	set(IN_NAME ${_name} CACHE STRING "" FORCE)
	set(IN_MAJOR ${_major} CACHE STRING "" FORCE)
	set(IN_MINOR ${_minor} CACHE STRING "" FORCE)
	set(IN_PATCH ${_patch} CACHE STRING "" FORCE)
	file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/version.h")
	configure_file("${CMAKE_SOURCE_DIR}/cmake/input/version.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/version.h")
	# Unset temporary cache variables
	unset(IN_NAME CACHE)
	unset(IN_MAJOR CACHE)
	unset(IN_MINOR CACHE)
	unset(IN_PATCH CACHE)
endfunction()