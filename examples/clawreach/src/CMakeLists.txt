file(GLOB_RECURSE UI_SRCS ./ui/*.c)

set(CLAWREACH_SRCS 
    "main.cpp"
    "websocket.cpp"
    "camera.cpp"
    "wifi.cpp"
    "media.cpp"
    "cmd.cpp"
    "qr_setup.cpp"
    ${UI_SRCS}
)

idf_component_register(
    SRCS ${CLAWREACH_SRCS}
    REQUIRES 
        driver
        esp_wifi
        nvs_flash
        esp_psram
        esp-libopus
        esp_websocket_client
        esp_http_client
        console
        json
    INCLUDE_DIRS "." "./ui"
)

# Suppress some warnings in dependencies
idf_component_get_property(lib esp-libopus COMPONENT_LIB)
target_compile_options(${lib} PRIVATE -Wno-error=maybe-uninitialized)
target_compile_options(${lib} PRIVATE -Wno-error=stringop-overread)
