#include "request.h"

void cmds :: build() {
 
    cmdList.insert(std::make_pair("set", 3)) ;
    cmdList.insert(std::make_pair("get", 2)) ;
    //验证命令
    cmdList.insert({"save", 1}) ;
    //-1代表不定参数
    cmdList.insert({"hset", -1}) ;
    cmdList.insert({"hget", 3}) ;
    cmdList.insert({"bgsave", 1}) ;
    cmdList.insert({"lpush", -1}) ;
    cmdList.insert({"lpop", 2}) ;
    cmdList.insert({"blpop", -1}) ;
    cmdList.insert({"zadd", 4}) ;
    cmdList.insert({"zrange", 4}) ;
    cmdList.insert({"sadd", -1}) ;
    cmdList.insert({"spop", -1}) ;
}

cmds :: cmds() {
}

cmds :: ~cmds() {}

int cmds ::isCmdExist(std::string cmd) {
    for(auto s : cmdList) {
        if(s.first == cmd) {
            return s.second ;
        }
    }
    return 0 ;
}

//匹配键值对
//创建命令表
int request :: processCmd(std::vector<std::string>&res, Messages::Command& com) {
    //创建命令集合
    static cmds cd ;
    cd.build() ;
    int len = res.size() ;
    if(!strcasecmp(res[0].c_str(), "set")) {
        //获取该命令的参数个数
        int ret = cd.cmdList[res[0]] ;
        if(len != ret) {
            cout << "error usage" << endl ;        
            return -1 ;
        }
        cmdProcess :: setSet(res, com) ;
    } 

    else if(!strcasecmp(res[0].c_str(), "lpush")) {
        cmdProcess :: getListObject(res, com) ;
        return 1;
    }

    //get命令
    else if(!strcasecmp(res[0].c_str(), "get")) {
        int ret = cd.cmdList[res[0]] ;
        if(ret != len) {
            std::cout << "error command!" << std::endl ;
            return -1 ;
        }   
        cmdProcess :: setGet(res, com) ;
    }
    else if(!strcasecmp(res[0].c_str(), "save")) {
        int ret = cd.cmdList[res[0]] ;
        if(ret != len) {
            std::cout << "error command!" << std::endl ;
            return -1 ;
        }
        cmdProcess :: setSave(res, com) ;
    }

    else if(!strcasecmp(res[0].c_str(), "hset")) {
        //不定参数,哈希表的总参数个数是偶数
        if(cd.cmdList[res[0]] != -1 || (len%2))  {
            std::cout << "error command!" << std::endl ;
            return -1 ;
        } 
        cmdProcess :: setHset(res, com) ;
    }

    else if(!strcasecmp(res[0].c_str(), "lpop")) {
        if(cd.cmdList[res[0]] != len) {
            std::cout << "error command" << std::endl ;
            return -1 ;
        }
        cmdProcess :: setLPopObject(res, com) ;      
    }

    else if(!strcasecmp(res[0].c_str(), "hget")) {
        int ret = cd.cmdList[res[0]] ;
        if(ret != len) {
            std::cout << "command error!" << std::endl ;
            return -1 ;
        }
        cmdProcess :: setHget(res, com) ;
    }
    else if(!strcasecmp(res[0].c_str(), "bgsave")) {
        int ret = cd.cmdList[res[0]] ; 
        if(ret != len) {
            std::cout << "command error!" << std::endl ;
            return -1 ;
        }
        cmdProcess :: setSave(res, com) ;
    }

    else if(!strcasecmp(res[0].c_str(), "blpop")) {
        int ret = cd.cmdList[res[0]] ;
    }
    else if(!strcasecmp(res[0].c_str(), "zadd")) {
        int ret = cd.cmdList[res[0]] ;  
        if(len < ret ) {
            std::cout << "command error!" << std::endl ;
            return -1 ;
        }
        cmdProcess::setZadd(res, com) ;
    }
    else if(!strcasecmp(res[0].c_str(), "sadd")) {
        int ret = cd.cmdList[res[0]] ;  
        if(ret != -1 ) {
            std::cout << "command error!" << std::endl ;
            return -1 ;
        }
        cmdProcess::setSadd(res, com) ;
    }

    else if(!strcasecmp(res[0].c_str(), "sPop")) {
        int ret = cd.cmdList[res[0]] ;  
        if(ret != -1 ) {
            std::cout << "command error!" << std::endl ;
            return -1 ;
        }
        cmdProcess::setSpop(res, com) ;
    }

    else if(!strcasecmp(res[0].c_str(), "zrange")) {
        int ret = cd.cmdList[res[0]] ;  
        if(len < ret ) {
            std::cout << "command error!" << std::endl ;
            return -1 ;
        }
        cmdProcess::setZRange(res, com) ;
    }

    else {
        std::cout << "command not found" << std::endl ;
        return -1 ;
    }
    return 1 ;
}


int request :: sendReq(int fd, std::vector<std::string>&res, int num) {
    Messages::Command cmd ;
    cmds cd ;
    cd.build() ;
    //没找到命令
    if(!cd.isCmdExist(res[0])) {
        std::cout << "command not found!"<< std::endl ;  
        return -1 ;
    }
    else {
        //从第一个数据
        int r = processCmd(res, cmd) ;
        if(r < 0) {
            return -1;
        }
    }    
    int s = sendAfterSerial(fd, cmd) ;
    return s ;
}

//判断是否与服务器断开连接
int request :: isConnect(int conFd) {
    
    struct tcp_info info ;
    int len = sizeof(info) ;
    int ret = getsockopt(conFd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t*)&len) ;
    if(ret < 0) {
        return -1 ;
    }
    //连接正常返回
    if(info.tcpi_state == TCP_ESTABLISHED) {
        return 1 ;
    }
    else {
        return 0 ;
    }
}

int request :: sendAfterSerial(int fd, Messages::Command& cmd) { 
    char buff[REQ_SIZE] ;
    bzero(buff, sizeof(buff)) ;
    std::string a ;
    if(clientLoop::mode == 0) {
        auto que = syncQueue::getQueue() ;
        long seq = que->addToQueue() ;
        std::string s = std::to_string(seq) ;
        cmd.set_seq(s) ;
        cmd.set_status(111) ;
    }

    //序列化的结果
    
    cmd.SerializeToArray(buff, REQ_SIZE) ;
    //检验是否与服务器器建立了
    int ret = isConnect(fd) ;
    if(ret == 0) {
        return 5 ;
    }

    if(send(fd, buff, sizeof(buff), 0) < 0) {
        std::cout << "errno connect    " << strerror(errno) << std::endl ;
        return -1;
    }
    cmd.clear_cmd() ;
    return 1 ;
}

