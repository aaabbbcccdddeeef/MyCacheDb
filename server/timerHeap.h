#ifndef	MYTIMER_H
#define	MYTIMER_H
#include <iostream>
#include <functional>
#include <vector>
#include "cmdProcess.h"
#include  "aeEpoll.h" 
#include "aeEventloop.h" 


class cmdProcess ; 
class TimerManager;
class aeEpoll ; 

class MyTimer {
    typedef std ::function <int(int fd)> wakeBlPopCall ;
    typedef std::function<int(std::map<int, std::shared_ptr<aeEvent>>&eventData, 
                              int, std::shared_ptr<aeEpoll>&aep)>Func; 
public :
    //访问aeEventloop中的对象
    static std::map<int, std::shared_ptr<aeEvent>>* data ;
    static std::shared_ptr<aeEpoll>aep ;
public:
    //循环还是只执行一次
	enum class TimerType{ONCE=0,CIRCLE=1};
    MyTimer (std::shared_ptr<TimerManager>&manager);
    ~MyTimer ();
	//启动一个定时器
    void start (Func func, unsigned int ms, TimerType type);
    void start (wakeBlPopCall func, unsigned int ms, TimerType type);
    void setFd(int fd) { 
        this->fd = fd ; 
    }
    int getFd() { 
        return fd ; 
    }
    //终止一个定时器
	void stop ();
    void setTimeSlot(int timer) {
        m_nInterval = timer ;
    }  
    int getIndex() { 
        return m_nHeapIndex  ;
    }
private :
	//执行
	void on_timer(unsigned long long now, int flag);
    void add_time(unsigned long long now) ;
private :
    
	friend class TimerManager;
    Func m_timerfunc ;
    wakeBlPopCall wakeFunc ;
	std::shared_ptr<TimerManager>& manager_ ;
	//调用函数，包括仿函数
	TimerType timerType_;
	//间隔
	unsigned int m_nInterval;
	//过期
	unsigned long long  m_nExpires;
	int  m_nHeapIndex;
    int fd ;
};
 
class TimerManager {
public :
    TimerManager() {}
    ~TimerManager() {
        heap_.clear() ;
    }
public:
	//获取当前毫秒数
	static unsigned long long get_current_millisecs();
	//探测执行
	void detect_timers(int i);
    void modify_timers(int fd) ;
    int getSize() ;
    void printTime(long now) ;
private :
	friend class MyTimer;
	//添加一个定时器
	void add_timer(MyTimer timer);
	//移除一个定时器
	void remove_timer(int index);
    void removeAll() ;
	//定时上浮
	void up_heap(size_t index);
	//定时下沉
	void down_heap(size_t index);
	//交换两个timer索引
    void swap_heap(size_t index1, size_t index2);
private:
    struct HeapEntry {
        unsigned long long time;
        std::shared_ptr<MyTimer>timer;
    };
    std::vector<HeapEntry> heap_;
};
 
#endif
