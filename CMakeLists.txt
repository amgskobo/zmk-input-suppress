# SPDX-License-Identifier: MIT
zephyr_include_directories(include dts)
if(CONFIG_ZMK_INPUT_SUPPRESS)
  target_sources(app PRIVATE src/input_processor_suppress.c)
endif()
