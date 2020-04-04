#include <boost/date_time/posix_time/posix_time.hpp>
#include <functional>
#include <condition_variable>
#include <boost/bind.hpp>
#include <functional>
#include <mutex>
#include <queue>


#include <nw_system/nw_event.h>
#include <nw_system/nw_main.h>
#include <nw_system/nw_service.h>

#include <nw_system/nw_unique.h>
NW_OBJ_DEF(event,event_ptr,[nw_event])
NW_OBJ_DEF(exec_event,exec_event_ptr,[nw_exec_event])
NW_OBJ_DEF(timeout_event,timeout_event_ptr,[nw_timeout_event])
NW_OBJ_DEF(interval_event,interval_event_ptr,[nw_interval_event])



_nw_event::~_nw_event() {
nw_object_ref_decrement(get_type());
}
_nw_exec_event::~_nw_exec_event()
{
    cb=nullptr;
}
_nw_timeout_event::~_nw_timeout_event()
{
    cb = nullptr;
    service = nullptr;
}
_nw_interval_event::~_nw_interval_event() {}


_nw_event::_nw_event(int t,std::string f,int l):_nw_object(),type(t),file(f),line(l)
{
    nw_object_ref_increment(get_type());
}

///** constructors *//
_nw_exec_event::_nw_exec_event(std::function<void()>c,std::string f,int l):_nw_event(0,f,l),cb(c) {}
_nw_timeout_event::_nw_timeout_event(std::function<void()>c,uint64_t milliseconds,nw_service_ptr svr,std::string f,int l):_nw_event(1,f,l),cb(c),
    id(nw_get_unique_id()),service(svr) {
    boost::posix_time::ptime when = boost::posix_time::microsec_clock::local_time()+boost::posix_time::milliseconds(milliseconds);
    nw_property_init<boost::posix_time::ptime>(property_list,"when",when);
    }
_nw_interval_event::_nw_interval_event(std::function<bool()>c,uint64_t milliseconds,nw_service_ptr loop,std::string f,int l):
    _nw_timeout_event(([&,c,loop]()->void
{
    try{
        if(cb)
        {
            if(c())/// check if this is ok
            {
              boost::posix_time::ptime  when = boost::posix_time::microsec_clock::local_time()+boost::posix_time::milliseconds(interval);
              nw_property_set<boost::posix_time::ptime>(property_list,"when",when);
              nw_service_set_interval(loop,shared_from_this());
            }
        }
        else std::cout <<CONSOLE_SYSTEM_WARN << "null interval callback\nsource: " << file << ":" << line << std::endl;
    }
    catch(const std::exception&ex)
    {
        std::cout << CONSOLE_SYSTEM_ERROR<< loop->name << " interval callback exception: "<< ex.what() << std::endl;
        std::cout << "source: "<< id << ":" << file<<":"<< line << std::endl;
    }
    catch(...)
    {
        std::cout << CONSOLE_SYSTEM_ERROR<<loop->name << " interval callback unknown exception"<< std::endl;
        std::cout << "source: " << id << ":" <<  file<<":"<< line << std::endl;
    }
}),milliseconds,loop,f,l),interval(milliseconds){}



///** methods *//
/// _nw_event
void _nw_event::do_action() {}
nw_event_ptr _nw_event::to_event() {
return shared_from_this();
}
uint64_t _nw_event::get_id(){
return 0;
}
/// _nw_exec_event
void _nw_exec_event::do_action()
{
    try
    {
        if(cb)
        {
            cb();
        }
        else std::cout << CONSOLE_SYSTEM_WARN<<"null exec callback\nsource: " << file << ":" << line << std::endl;
    }
    catch(const std::exception&ex)
    {
        std::cout << CONSOLE_SYSTEM_ERROR<<"exec callback exception: "<< ex.what() << std::endl;
        std::cout << "source: " << file<<":"<< line << std::endl;
    }
    catch(...)
    {
        std::cout << CONSOLE_SYSTEM_ERROR<<"exec callback unknown exception " << std::endl;
        std::cout << "source: " << file<<":"<< line << std::endl;
    }
}
nw_event_ptr _nw_exec_event::to_event() {
return shared_from_this();
}

/// _nw_timeout_event
void _nw_timeout_event::do_action()
{
    try
    {
        if(cb)
        {
            cb();
        }
        else std::cout << CONSOLE_SYSTEM_WARN <<"null timeout callback\nsource: " << file << ":" << line << std::endl;
    }
    catch(const std::exception&ex)
    {
        std::cout << CONSOLE_SYSTEM_ERROR<<"timeout callback exception: "<< ex.what() << std::endl;
        std::cout << "source: " << file<<":"<< line << std::endl;
    }
    catch(...)
    {
        std::cout << CONSOLE_SYSTEM_ERROR<<"timeout callback unknown exception: " << id << std::endl;
        std::cout << "source: " << file<<":"<< line << std::endl;
    }
}
nw_event_ptr _nw_timeout_event::to_event() {
return shared_from_this();
}
uint64_t _nw_timeout_event::get_id(){
return id;
}
nw_event_ptr _nw_interval_event::to_event() {
return shared_from_this();
}
/** exports */
nw_exec_event_ptr nw_exec_event_new(std::function<void()>cb,std::string file,int line)
{
    nw_exec_event_ptr ptr = nw_exec_event_ptr(new _nw_exec_event(cb,file,line));
    return ptr;
}
nw_timeout_event_ptr nw_timeout_event_new(nw_service_ptr svr,std::function<void()>cb,uint64_t milliseconds,std::string file,int line)
{
    nw_timeout_event_ptr ptr = nw_timeout_event_ptr(new _nw_timeout_event(cb,milliseconds,svr,file,line));
    return ptr;
}
nw_interval_event_ptr nw_interval_event_new(nw_service_ptr loop,std::function<bool()>cb,uint64_t milliseconds,std::string file,int line)
{
    nw_interval_event_ptr ptr = nw_interval_event_ptr(new _nw_interval_event(cb,milliseconds,loop,file,line));
    return ptr;
}

NW_PUBLIC void nw_event_do_action(nw_event_ptr ptr){
if(ptr){
ptr->do_action();
}else NW_NULL_OBJECT_WARNING
}
