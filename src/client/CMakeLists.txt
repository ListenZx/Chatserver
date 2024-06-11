#把当前路径下所有源文件导入
aux_source_directory(. SRC_LIST)

#指定生成可执行文件
add_executable(chatclient ${SRC_LIST})

#链接地址
target_link_libraries(chatclient pthread)

