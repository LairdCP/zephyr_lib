# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LCZ)

add_subdirectory(ble_common)
add_subdirectory(ble_services)
add_subdirectory(components)
add_subdirectory(utilities)
add_subdirectory(mcumgr_wrapper)
add_subdirectory(drivers)
add_subdirectory(memfault)
add_subdirectory(mcumgr_cmds)
add_subdirectory(lcz_kvp)

endif()

# Include local project version output
include(cmake/git_version.cmake)
