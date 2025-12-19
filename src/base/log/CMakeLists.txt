add_library(log STATIC
    Logger.cpp
)

target_include_directories(log
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(log
    PUBLIC
        spdlog::spdlog
        project_options
)