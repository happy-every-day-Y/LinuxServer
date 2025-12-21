add_library(eventloop STATIC
    EventLoop.cpp)

target_include_directories(eventloop
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(eventloop
    PUBLIC
        project_options
        log
)