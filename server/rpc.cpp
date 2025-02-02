#include "rpc.h"

rpc :: rpc()  {
    cmd = std::shared_ptr<Messages::Command>(new Messages::Command) ;
    parseMethod = requestMethod ;
}


//处理结果，并返回相应的结果
std::shared_ptr<Messages::Command> rpc :: getParseString(const char* buf) {
    //在消息处理处，反序列化
    auto res = parseMethod(buf) ;
    return res ;
}


int rpc :: response(std::shared_ptr<Messages::Response>res, int fd) {
    //回复客户端
    //序列化，转化成string
    char buf[4096] ;
    res->SerializeToArray(buf, 4096) ;
    //向客户端发送消息
    int ret = send(fd, buf, sizeof(buf), 0) ;
    if(ret < 0) {
        cout << __FILE__ << "      " << __LINE__ << "  " << strerror(errno)<< endl ;
        return -1 ;
    }
    return ret ;
}

///反序列化
std::shared_ptr<Messages::Command> requestMethod(const char* buf) {
    Messages::Command cmd ;
    cmd.ParseFromArray(buf, 4096) ;
    std::shared_ptr<Messages::Command>comm = make_shared<Messages::Command>(cmd) ;
    return comm ;
}
