# SPDX-License-Identifier: Apache-2.0

#.rst:
# git_version.cmake
# ---------
# Adapted from cmake/git.cmake in Zephyr.
# If the user didn't already define BUILD_VERSION_LOCAL then try to
# initialise it with the output of "git describe". Warn but don't
# error if everything fails and leave BUILD_VERSION_LOCAL undefined.

# https://cmake.org/cmake/help/latest/module/FindGit.html
find_package(Git QUIET)
if(NOT DEFINED BUILD_VERSION_LOCAL AND GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --abbrev=12 --always
    WORKING_DIRECTORY                ${CMAKE_BINARY_DIR}
    OUTPUT_VARIABLE                  BUILD_VERSION_LOCAL
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE                   stderr
    RESULT_VARIABLE                  return_code
  )
  if(return_code)
    message(STATUS "git describe failed: ${stderr}")
  elseif(NOT "${stderr}" STREQUAL "")
    message(STATUS "git describe warned: ${stderr}")
  endif()

if(DEFINED BUILD_VERSION_LOCAL)
  message(STATUS "Application version: ${BUILD_VERSION_LOCAL}")
  zephyr_compile_definitions(
    BUILD_VERSION_LOCAL=${BUILD_VERSION_LOCAL}
  )
else()
  message(STATUS "Application version could not be determined")
endif()
endif()
