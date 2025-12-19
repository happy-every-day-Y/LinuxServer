file(GLOB MODEL_SOURCES *.cpp)

add_library(model STATIC ${MODEL_SOURCES})

target_include_directories(model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(model
    PUBLIC
        project_options
        log
)
