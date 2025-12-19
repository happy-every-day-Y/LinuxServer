add_library(buffer STATIC
    Buffer.cpp)

target_include_directories(buffer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(buffer
    PUBLIC
        project_options
        log
)