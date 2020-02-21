idf_component_register(SRCS "mqtt_start.c"
                    INCLUDE_DIRS "." $ENV{IDF_PATH}/examples/common_components/protocol_examples_common
                    PRIV_REQUIRES mqtt nvs_flash)