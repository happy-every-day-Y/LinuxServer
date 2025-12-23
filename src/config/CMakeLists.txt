add_library(config STATIC Config.cpp)

target_include_directories(config PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(config
    PUBLIC
        project_options
        log
        json
)