# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_DGUS2_DRIVER
  src/dgus2_protocol.c
  src/dgus2_hal.c
  src/dgus2_core.c
)

zephyr_library_include_directories(include)
zephyr_include_directories(include)
