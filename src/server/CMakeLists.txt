#把当前路径下所有源文件导入
aux_source_directory(. SRC_LIST )
#把当前路径下db的源文件导入
aux_source_directory(./db DB_LIST )
#把当前路径下model的源文件导入
aux_source_directory(./model MODEL_LIST )
#把当前路径下redis的源文件导入
aux_source_directory(./redis REDIS_LIST )
#指定生成可执行文件
add_executable(chatserver ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

#链接地址
target_link_libraries(chatserver muduo_net muduo_base mysqlclient pthread hiredis)