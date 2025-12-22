add_library(connection STATIC
    Connection.cpp HttpConnection.cpp WebSocketConnection.cpp)

target_include_directories(connection
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(connection
    PUBLIC
        project_options
        log
        session
)