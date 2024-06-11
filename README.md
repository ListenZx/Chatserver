# Chatserver
基于muduo、工作在Nginx tcp负载均衡环境中、使用redis消息队列可跨服务器通信 的聊天服务器和客户端源码


编译方式
cd build 
rm -rf *
cmake ..
make

环境：Nginx负载均衡，muduo库，mysql，redis
