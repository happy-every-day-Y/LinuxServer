file(GLOB LOGIC_SOURCES *.cpp)

add_library(logic STATIC ${LOGIC_SOURCES})

target_include_directories(logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(logic
    PUBLIC
        project_options
        log
        config
        dao
)
