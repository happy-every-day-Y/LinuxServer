add_library(mysql STATIC
    MysqlConn.cpp MysqlPool.cpp
)

target_include_directories(mysql
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mysql
    PUBLIC
        project_options
        log
        config
        mysqlclient
)