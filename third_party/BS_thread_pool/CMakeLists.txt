# third_party/thread_pool/CMakeLists.txt
add_library(BS_thread_pool INTERFACE)

target_include_directories(BS_thread_pool INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
