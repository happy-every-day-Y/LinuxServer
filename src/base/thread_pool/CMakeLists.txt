add_library(thread_pool STATIC
    Thread_pool.cpp
)

target_include_directories(thread_pool
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(thread_pool
    PUBLIC
        project_options
        log
        BS_thread_pool
)