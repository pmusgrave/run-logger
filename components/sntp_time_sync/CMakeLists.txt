idf_component_register(SRCS "sntp_time_sync.cpp"
                    INCLUDE_DIRS "include" $ENV{IDF_PATH}/examples/common_components/protocol_examples_common PRIV_REQUIRES nvs_flash)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
