cmake_minimum_required(VERSION 3.10)
project(TinyWebServer)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加源文件
add_executable(test
    test.cpp
    log/log.cpp
    poll/sqlconnpoll.cpp
    poll/threadPool.h
    buffer/buffer.cpp
    buffer/blockqueue.h
)

# 包含头文件目录
target_include_directories(test PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/log
    ${CMAKE_SOURCE_DIR}/poll
    ${CMAKE_SOURCE_DIR}/buffer
    /usr/include/mysql
)

# 链接MySQL库和pthread
target_link_libraries(test
    mysqlclient
    pthread
)