# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/common) #set duong dan den common
project(wifi_station)



target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "image/anhmau.jpeg" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "image/anhmau2.jpeg" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "html/index.html" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "html/index2.html" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "html/index3.html" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "html/index4.html" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "html/index5.html" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "data/all.css" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "data/jquery-3.6.0.min.js" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "data/jspdf.plugin.autotable.min.js" TEXT)
target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "data/jspdf.umd.min.js" TEXT)