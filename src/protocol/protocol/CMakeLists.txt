add_library(protocol STATIC
    HttpRequest.cpp HttpResponse.cpp WebSocketFrame.cpp Message.h)

target_include_directories(protocol
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(protocol
    PUBLIC
        project_options
        log
        buffer
)