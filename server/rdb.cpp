#include "rdb.h"

std::string rdb :: tmpFileName(const char* fileName) {
    std::string tmp = fileName ;
    tmp += ".tmp" ;
    return tmp ;
}
//保存到文件
int rdb :: save(const std::shared_ptr<redisDb> db, char* fileName) {     
    //获取数据库文件
    std::string ss = tmpFileName(fileName) ;  
    ofstream out(ss, ios::out|ios::binary|ios::trunc) ;
    if(out.fail()) {
        std::string s = "         " +std::to_string(__LINE__) +__FILE__+strerror(errno) ;
        aofRecord::log(s) ;
        return -1;
    }
    int num = db->getId() ;
    //数据库编号
    std::string head = makeHeader() ;
    //将头部写入文件
    const char* h =head.c_str() ;
    out << h ;
    out << "id:" << num <<"\r\n" ;
    //获取数据库中的对象元素
    std::shared_ptr<dbObject> rd = db->getNextDb() ;
    while(rd != nullptr) {
        std::string value ;
        int type = rd->getType() ;
        //字符串类别是string
        out << "e:" ;
        out << rd->getEndTime() << "\r\n" ;
        if(type == type::DB_STRING) {
            std::string value = rd->getValue() ; 
            std::string key = rd->getKey() ;
            out << "ctp:" << type::DB_STRING << "\r\n" ;
            processString(key, out, value) ;
        }
        //hash的保存
        if(type == type::DB_HASH) {
            std::string key = rd->getKey() ;
            out << "ctp:" <<type:: DB_HASH << "\r\n" ;
            processHash(out, rd) ;         
        }
        //是链表
        if(type == type::DB_LIST) {
          //  out << rd->getEndTime() << "\r\n" ;
            out  <<"ctp:" << type::DB_LIST << "\r\n" ;
            processList(out, rd) ;
        }   
        rd = db->getNextDb() ;
    }
    out << "\r\n" ;  
    out.close() ;
    remove(fileName) ;
    rename(ss.data(), fileName) ;
    return 1 ;
}

void rdb :: processList(ofstream& aa, const std::shared_ptr<dbObject>rd) {
    std::string key = rd->getKey() ;
    aa << key << "\r\n" ;
    std::vector<std::string> ls = rd->getValues("") ;
    int len = ls.size() ;
    for(int i=0; i<len; i++) {
        aa <<ls[i]<< "\r\n" ;
    }
    aa << "\r$\n" ;
}


void rdb :: processHash(ofstream& out, const std::shared_ptr<dbObject>rd) {

    std::string key = rd->getKey() ;
    std::string value="" ;
    int len = 0 ;
    out << key << "\r\n" ;
    //获取hash中的键值属性
    while(1) {
        value = rd->getValue() ;
        len =value.size() ;
        if(len == 0) {
            break ;
        }
        int index = value.find("\r\n") ;
        std::string key = value.substr(0, index) ;
        std::string val = value.substr(index+2, len-index) ;
        out << key << ":" << val << "\r\n" ;
    }
    out <<"\r$\n" ;
}

void rdb :: processString(const std::string key, ofstream& out, const std::string value) {

    const char * v = value.c_str() ;
    //编码类型
    int type = getStringEncodingType(value) ;
    //值是以字符串，使用raw编码
    //分为两种编码形式，一种是小于20字节的原样保存
    //一种是大于20字节的压缩保存
    if(type == ENCODING_INT::INT8) {
        //编码形式
        out << "ec:" << STRING :: REDIS_ENCODING_INT << "\r\n" ;
        //值
        int8_t a = atoi(v) ;
        //将键值写入到文件中
        out << key <<":" << a << "\r$\n" ;
    }   

    else if(type == ENCODING_INT::INT16) {
        out << "ec:" << STRING :: REDIS_ENCODING_INT << "\r\n" ;
        //值
        int16_t a = atoi(v) ;
        //将键值写入到文件中
        out << key <<":" << a << "\r$\n" ;
    }   
    else if(type == ENCODING_INT::INT32) {
        out << "ec:" << ENCODING_INT::INT8 << "\r\n" ;
        //值
        int32_t a = atoi(v) ;
        //将键值写入到文件中
        out << key <<":" << a << "\r$\n" ;
    }   
    else if(type == STRING::REDIS_ENCODING_RAW) {
        out << "ec:" << STRING::REDIS_ENCODING_RAW << "\r\n" ;
        //值
        int size = value.size() ;
        if(size <= 20) {
            out << "yc:" << size <<"\r\n" ;
            out << key <<":" << value<< "\r$\n";                     
        }   
        else {
            out << "yc:" << size << "\r\n" ;
            //原长写入文件
            //标志是否压缩过
            //压缩值
            //将字符串原长保存到文件
            int ll = 0 ; 
            
            string a = lzfCompress(value, ll);
            if(a.empty()) {
                ll = value.size() ;
                a = value ;
            }
            out << "xc:" << ll << "\r\n" ;
            out << key << ":" ;
            out << a <<"\r$\n" ;
        }
    }   
}

std::string rdb :: lzfCompress(std::string value, int& ll) {
    size_t len = value.size();  // 字符串未压缩前的长度
    size_t comprlen;  // 压缩后的长度
    size_t outlen;    // 输出缓存的最大长度
    void *out;
    //当字节至少四字节长以上时才能压缩
    if (len <= 4) {
        return "";
    }
    
    outlen = len-4;
    
    if ((out = malloc(outlen+3)) == NULL)  {
        return "";
    }
    //传入数据，数据长度，任意类型的指针
    comprlen = lzf_compress(value.data(), len, out, outlen);  
    if (comprlen == 0) { 
        free(out);
        return "";
    }
    ll = comprlen ;
    std::string a = (char*)out ;
    free(out) ;
    return a ;
}

int rdb :: getStringEncodingType(const std::string value) {

    if(isNum(value.c_str())) {
        //如果int小于32为整数
        if(atoll(value.c_str()) < INT32_MAX) {
            int a = atoi(value.c_str()) ;
            if(a < INT8_MAX) {
                return ENCODING_INT::INT8 ;
            }
            else if(a < INT16_MAX) {
                return ENCODING_INT::INT16 ;
            }
            else {
                return ENCODING_INT::INT32 ;
            }
        }
    }
    //以上都不满足,返回raw编码
    return STRING::REDIS_ENCODING_RAW ;
}

//创建rdb头部,redis标识，版本号
std::string rdb :: makeHeader() {
    std::string head ;
    //rdb文件标志
    head += "redis" ;
    head += "0001\r\n" ;   
    //rdb中database部分
    head += flag ;
    return head ;
}

bool rdb :: isNum(const char* num) {
    regex regFloat("^(-?\\d+)(\\.\\d+)?$");
    regex regInt("^-?\\d+$") ;
    bool res = regex_match(num, regFloat);
    if(res) {
        return res ;
    }   
    res = regex_match(num, regInt);
    if(res) {
        return res ;
    }   
    return false ;
}

int rdb :: getAllFileName(std::vector<std::string>&nameLs) {
    ifstream ifs(".rdb/.redis_fileName", ios::in) ;
    if(ifs.fail()) {
        std::string s = "         " +std::to_string(__LINE__) +__FILE__+strerror(errno) ;
        aofRecord::log(s) ;
        return -1 ;
    }
    while(!ifs.eof()) {
        std::string file ;
        ifs >> file ;
        nameLs.push_back(file) ;
    }
    ifs.close() ;
    return 1 ;
}


std::string rdb :: getFileInfo(const std::string s) {
    int fd = open(s.c_str(), O_RDWR) ;
    if(fd < 0) {
        std::string s = "         " +std::to_string(__LINE__) +__FILE__+strerror(errno) ;
        aofRecord::log(s) ;
        return "" ;
    }
    struct stat st ;
    int ret = fstat(fd, &st) ;
    if(st.st_size == 0) {
        std::string s = "         " +std::to_string(__LINE__) +__FILE__+strerror(errno) ;
        aofRecord::log(s) ;
        return "" ;
    }
    if(ret < 0) {
        std::string s = "         " +std::to_string(__LINE__) +__FILE__+strerror(errno) ;
        aofRecord::log(s) ;
        return "" ;
    }
    long size = st.st_size ;
    if(size%4096 != 0) {
        size = (size/4096+1)*4096 ;
    }
    //内存映射
    char* flag = (char*)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0) ;
    if(flag == NULL) {
        std::string s = "         " +std::to_string(__LINE__) +__FILE__+strerror(errno) ;
        aofRecord::log(s) ;
        return "" ;
    }
    close(fd) ;
    std::string b = flag ;
    munmap(flag, size) ;
    return b ;
}

//初始化数据库
int rdb :: initRedis(cmdSet* cmdset) {
    std::vector<std::string>nameLs ;
    int i = getAllFileName(nameLs) ;
    if(nameLs.size() == 0|| i < 0) {
        return -1 ;
    }
    for(auto s: nameLs) {
        if(s.size() == 0) {
            continue ;
        }
        std::string str = getFileInfo(s) ;   
        if(str.empty()) {
            return -1 ;
        } 
        std::shared_ptr<redisDb>db = recoverDb :: recover(str, cmdset) ;   
    }
    return 1 ;
}

std::string rdb::readLogFile(const std::string& file) {
    int fd = open(file.c_str(), O_RDONLY);
    if(fd < 0) {
        std::string s = "         " +std::to_string(__LINE__) +__FILE__+strerror(errno) ;
        aofRecord::log(s) ;
        return "" ;
    }
    struct stat st ;
    fstat(fd, &st) ;
    char* buf = (char*)mmap(buf, st.st_size, PROT_READ, 0, fd, 0) ;
    if(buf == NULL) {
        close(fd) ;
        munmap(buf, st.st_size) ;
        return "" ;
    }
    std::string ss = buf ;
    munmap(buf, st.st_size) ;
    close(fd) ;
    return ss ;
}

int rdb::getLogFileName(std::vector<std::string>&logName) {
    ifstream in("../logInfo/allLogFileName", ios::in) ;
    if(in.fail()) {
        std::string s = "         " +std::to_string(__LINE__) +__FILE__+strerror(errno) ;
        aofRecord::log(s) ;
        return -1 ;   
    }
    while(!in.eof()) {
        std::string s = "" ;
        in>> s ;
        logName.push_back(s) ;
    }
    in.close() ;
    return 1 ;
}
