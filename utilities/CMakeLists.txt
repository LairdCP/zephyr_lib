#
# Laird Connectivity Zephyr Utilities
#
zephyr_include_directories(include)

zephyr_sources(source/hexcode.c)
zephyr_sources(source/string_util.c)
zephyr_sources(source/print_json.c)
zephyr_sources_ifdef(CONFIG_PRINT_THREAD_LIST source/print_thread.c)
