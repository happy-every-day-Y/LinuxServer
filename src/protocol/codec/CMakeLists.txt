add_library(codec STATIC
    HttpCodec.cpp Codec.h WebSocketCodec.cpp)

target_include_directories(codec
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(codec
    PUBLIC
        project_options
        log
        buffer
        protocol
)