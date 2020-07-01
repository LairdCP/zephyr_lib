#
# Laird Connectivity Zephyr Components
#
zephyr_include_directories(include)

zephyr_sources_ifdef(CONFIG_MCUMGR source/mcumgr_wrapper.c)
zephyr_sources_ifdef(CONFIG_SIMPLE_BLUETOOTH source/simple_bluetooth.c)

if(CONFIG_MCUMGR)
zephyr_link_libraries(MCUMGR INTERFACE
  zephyr_interface
  )
endif()

if(CONFIG_FILE_SYSTEM_LITTLEFS)
zephyr_link_libraries(LITTLEFS INTERFACE
  zephyr_interface
)
endif()
