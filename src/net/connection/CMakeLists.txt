add_library(connection STATIC
    TcpConnection.cpp)

target_include_directories(connection
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(connection
    PUBLIC
        project_options
        log
        buffer
        reactor
)