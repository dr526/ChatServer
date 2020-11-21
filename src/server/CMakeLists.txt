#设置依赖项
#定义了SRC_LIST变量，包含该目录下所有的源文件
aux_source_directory(. SRC_LIST)

#定义了DB_LIST变量，包含该目录下的db目录下所有的源文件
aux_source_directory(./db DB_LIST)

#定义MODEL_LIST变量，包含该目录下的model目录下所有的源文件
aux_source_directory(./model MODEL_LIST)

#定义一个SRC_LIST变量，包含了该目录下所有的源文件
aux_source_directory(./redis REDIS_LIST)

#生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

#指定可执行文件链接时需要依赖的库文件
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis pthread)
