cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g) #编译选项
project(main)

#头文件查好路径
include_directories( ${PROJECT_SOURCE_DIR}/include) #公共的
include_directories( ${PROJECT_SOURCE_DIR}/include/server) #自己的
include_directories( ${PROJECT_SOURCE_DIR}/thirdparty) #第三方库
include_directories( ${PROJECT_SOURCE_DIR}/include/server/db) #db
include_directories( ${PROJECT_SOURCE_DIR}/include/server/model) #model
include_directories( ${PROJECT_SOURCE_DIR}/include/server/redis) #redis
#可执行文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#搜索下一层 
add_subdirectory(src)