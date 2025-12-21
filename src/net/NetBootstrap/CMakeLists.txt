add_library(netbootstrap STATIC
    NetBootstrap.cpp)

target_include_directories(netbootstrap
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(netbootstrap
    PUBLIC
        project_options
        log
        acceptor
        eventloop
        session
        connection
)