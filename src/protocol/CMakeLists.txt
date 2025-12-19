file(GLOB PROTO_SOURCES *.cpp)

add_library(protocol STATIC ${PROTO_SOURCES})

target_include_directories(protocol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(protocol
    PUBLIC
        project_options
        log
)
