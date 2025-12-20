add_library(handler STATIC
    Handler.h HttpHandler.cpp)

target_include_directories(handler
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(handler
    PUBLIC
        project_options
        log
        connection
        protocol
        session
)