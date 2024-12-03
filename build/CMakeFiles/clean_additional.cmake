# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "all.css.S"
  "anhmau.jpeg.S"
  "anhmau2.jpeg.S"
  "bootloader\\bootloader.bin"
  "bootloader\\bootloader.elf"
  "bootloader\\bootloader.map"
  "config\\sdkconfig.cmake"
  "config\\sdkconfig.h"
  "esp-idf\\esptool_py\\flasher_args.json.in"
  "esp-idf\\mbedtls\\x509_crt_bundle"
  "flash_app_args"
  "flash_bootloader_args"
  "flash_project_args"
  "flasher_args.json"
  "index.html.S"
  "index2.html.S"
  "index3.html.S"
  "index4.html.S"
  "index5.html.S"
  "jquery-3.6.0.min.js.S"
  "jspdf.plugin.autotable.min.js.S"
  "jspdf.umd.min.js.S"
  "ldgen_libraries"
  "ldgen_libraries.in"
  "project_elf_src_esp32.c"
  "wifi_station.bin"
  "wifi_station.map"
  "x509_crt_bundle.S"
  )
endif()
