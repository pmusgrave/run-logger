# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS "./components" $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)

set(TEST_COMPONENTS "run" "runlogger" "wifi" "sntp_time_sync" "mqtt_start" "minmea" CACHE STRING "List of components to test")

set(SUPPORTED_TARGETS esp32)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wifi_station)
