add_library(bootstrap STATIC
    NetBootstrap.cpp)

target_include_directories(bootstrap
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(bootstrap
    PUBLIC
        project_options
        log
        reactor
        connection
        acceptor
)