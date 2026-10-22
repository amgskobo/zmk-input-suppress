if(CONFIG_INPUT_SUPPRESS_NATIVE_TEST)
  target_sources(app PRIVATE main.c)
  zephyr_link_libraries(-Wl,--wrap=zmk_endpoint_send_mouse_report)
endif()
