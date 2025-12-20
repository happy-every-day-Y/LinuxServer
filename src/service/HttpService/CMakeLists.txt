add_library(httpservice STATIC
    HttpService.cpp)

target_include_directories(httpservice
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(httpservice
    PUBLIC
        project_options
        log
        codec
        connection
        protocol
        session
        thread_pool
        handler
        dispatcher
)