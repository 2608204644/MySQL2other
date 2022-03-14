cmake_minimum_required(VERSION 3.18)
project(mysql2other)

set(CMAKE_CXX_STANDARD 14)

find_package(glog 0.4.0 REQUIRED)

add_executable(mysql2redis main.cpp config.cpp config.h filter.cpp filter.h event.cpp event.h mysql_connector.cpp mysql_connector.h util.cpp util.h reserve_data.cpp reserve_data.h redis_connector.cpp redis_connector.h line_data.cpp line_data.h packet.cpp packet.h thread_pool.cpp thread_pool.h my_decimal.h mysql_type_define.h memblock.cpp memblock.h event_dispatcher.cpp event_dispatcher.h factory.cpp factory.h my_decimal.cpp)

target_link_libraries(mysql2redis yaml-cpp.a glog::glog ssl crypto hiredis mysqlclient)
