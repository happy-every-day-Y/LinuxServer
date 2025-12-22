file(GLOB DAO_SOURCES *.cpp)

add_library(dao STATIC ${DAO_SOURCES})

target_include_directories(dao PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(dao
    PUBLIC
        project_options
        log
        config
)