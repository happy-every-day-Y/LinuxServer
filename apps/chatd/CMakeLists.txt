add_executable(chatd main.cpp)

target_link_libraries(chatd
    PRIVATE
        project_options
        log
        config
        thread_pool
        netbootstrap
        acceptor
        session
        mysql
)
