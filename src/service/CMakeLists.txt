file(GLOB SERVICE_SOURCES *.cpp)

add_library(service STATIC ${SERVICE_SOURCES})

target_include_directories(service PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(service
    PUBLIC
        project_options
        log
        config
        dao
        logic
)
