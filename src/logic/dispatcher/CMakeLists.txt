add_library(dispatcher STATIC
    Dispatcher.cpp)

target_include_directories(dispatcher
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dispatcher
    PUBLIC
        project_options
        log
        codec
        session
        handler
        protocol
)