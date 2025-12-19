add_library(acceptor STATIC
    Acceptor.cpp)

target_include_directories(acceptor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(acceptor
    PUBLIC
        project_options
        log
        reactor
)