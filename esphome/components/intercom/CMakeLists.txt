idf_component_register(
    SRCS
        "intercom.cpp"
    INCLUDE_DIRS
        "."
    REQUIRES
        esp_http_client
        esp_websocket_client
        json
        mbedtls
        esp_peer
        esp_webrtc
        # Add other ESP WebRTC dependencies as needed
)

# If ESP WebRTC Solution is in parent directory
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../esp-webrtc-solution")
    set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../esp-webrtc-solution/components")
endif()

