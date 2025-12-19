add_library(reactor STATIC
    EventLoop.cpp Channel.cpp EpollPoller.cpp)

target_include_directories(reactor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(reactor
    PUBLIC
        project_options
        log
)