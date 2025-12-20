add_library(session STATIC
    Session.cpp SessionManager.cpp)

target_include_directories(session
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(session
    PUBLIC
        project_options
        log
        connection
)