#include <boost/date_time/posix_time/posix_time.hpp>
#include <condition_variable>
#include <boost/bind.hpp>
#include <functional>
#include <mutex>
#include <queue>

#include <nw_system/base.h>
#include <nw_system/nw_object.h>
#include <nw_system/nw_event.h>
#include <nw_system/nw_service.h>
#include <nw_system/nw_main.h>
#include <nw_system/nw_unique.h>
NW_OBJ_DEF(service,service_ptr,[nw_service])

/** exports */
NW_PUBLIC nw_service_ptr nw_service_new(std::string name)
{
    nw_service_ptr ptr = nw_service_ptr(new _nw_service(name));
    nw_object_ref(ptr);
    return ptr;
}
NW_PUBLIC  bool nw_is_service(nw_object_ptr ptr)
{
    NW_OBJECT_RETURN_BOOL_IS(service,ptr);
}
NW_PUBLIC  nw_service_ptr nw_service_from_object(nw_object_ptr ptr)
{
    if(!ptr)
    NW_NULL_OBJECT_WARNING
    if(nw_is_service(ptr))
        return ((_nw_service*)ptr.get())->shared_from_this();
    return nullptr;
}
NW_PUBLIC  nw_object_ptr nw_service_to_object(nw_service_ptr ptr)
{
    if(!ptr){
     NW_NULL_OBJECT_WARNING
        return nullptr;
    }
    return ptr->to_object();
}
NW_PUBLIC  int nw_service_run(nw_service_ptr ptr,int interval)
{
    if(ptr){
    return ptr->run(interval);
    }
    else NW_NULL_OBJECT_WARNING
        return 0;
}
NW_PUBLIC  int nw_service_run_once(nw_service_ptr ptr)
{
    if(ptr){
    return ptr->run_once();
    }else NW_NULL_OBJECT_WARNING
    return 0;
}

NW_PUBLIC  int nw_service_run_one_only(nw_service_ptr ptr)
{
    if(ptr){
    return ptr->run_one_only();
    }else NW_NULL_OBJECT_WARNING
    return 0;
}
NW_PUBLIC  void nw_service_stop(nw_service_ptr ptr,int reason,std::function<void(int)>cb)
{
    if(ptr)
    {
    ptr->stop(reason,cb);
    }else NW_NULL_OBJECT_WARNING
}
NW_PUBLIC  void nw_service_exec(nw_service_ptr ptr,std::function<void()>cb,std::string source, int line)
{
    if(ptr){
    ptr->exec(cb,source,line);
    }else NW_NULL_OBJECT_WARNING
}
NW_PUBLIC  uint64_t nw_service_set_timeout(nw_service_ptr ptr,std::function<void()>cb,uint64_t milliseconds,std::string source, int line)
{
    if(ptr){
    return ptr->set_timeout(cb,milliseconds,source,line);
    }else
    NW_NULL_OBJECT_WARNING
    return 0;
}
NW_PUBLIC  uint64_t nw_service_set_interval(nw_service_ptr ptr,std::function<bool()>cb,uint64_t milliseconds,std::string source, int line)
{
    if(ptr){
    return ptr->set_interval(cb,milliseconds,source,line);
    }else NW_NULL_OBJECT_WARNING
    return 0;
}
void nw_service_set_interval(nw_service_ptr ptr,nw_interval_event_ptr evt){
if(ptr){
  ptr->set_interval(evt);
}else NW_NULL_OBJECT_WARNING
}
NW_PUBLIC  void nw_service_clear_timeout(nw_service_ptr ptr,uint64_t id)
{
    if(ptr){
    ptr->clear_timeout(id);
    }else NW_NULL_OBJECT_WARNING
}
NW_PUBLIC  void nw_service_clear_interval(nw_service_ptr ptr,uint64_t id)
{
    if(ptr){
    ptr->clear_interval(id);
    }else NW_NULL_OBJECT_WARNING
}
NW_PUBLIC  void nw_service_push_event(nw_service_ptr ptr,nw_event_ptr&evt)
{
    if(ptr){
    ptr->push_event(evt);
    }else NW_NULL_OBJECT_WARNING
}
NW_PUBLIC  void nw_service_set_persistent(nw_service_ptr ptr,bool b)
{
    if(ptr){
    ptr->set_persistent(b);
    }else NW_NULL_OBJECT_WARNING
}
NW_PUBLIC  bool nw_service_get_persistent(nw_service_ptr ptr)
{
    if(ptr){
    return ptr->get_persistent();
    }else NW_NULL_OBJECT_WARNING
    return false;
}
/** end of exports */


_nw_service::_nw_service(std::string n):_nw_object(),name(n),id(nw_get_unique_id()),
    stop_request(false),persistent(true),normal_run(false),finished(false),exit_code(0),
    event_list(),ready_handlers_list(),timeout_list(),timeout_clear_request(),prop_mutex(),event_mutex(),event_cond()
{
    nw_object_ref_increment(get_type());
}
_nw_service::~_nw_service()
{
    nw_object_ref_decrement(get_type());
    event_mutex.lock();
    while(!event_list.empty())
    {
        event_list.pop();
    }
    auto t = timeout_list.begin();
    while(t != timeout_list.end())
    {
        timeout_list.erase(t);
        t = timeout_list.begin();
    }
    event_mutex.unlock();
    stop_callback = nullptr;
}





int _nw_service::run(int interval)
{
    if(get_run_mode()){
     return -1;
    }
    set_run_mode(true);
    set_finished(false);
    stop_request = false;
    exit_code = 0;
    while(!stop_request)
    {
        {
            std::unique_lock<std::mutex> lock(event_mutex);
            while(event_list.empty())
            {
                if(event_cond.wait_for(lock,std::chrono::milliseconds(interval))==std::cv_status::timeout)
                {
                    if(timeout_list.size()>0){
                        break;
                    }
                }
            }
            lock.unlock();
            run_once();
        }
        if(!get_persistent())
        {
            event_mutex.lock();
            if(event_list.empty()&&timeout_list.size()==0)
            {
                stop_request = true;
            }
            event_mutex.unlock();
        }
    }
    set_finished(true);
    set_run_mode(false);

    stop_request = true;
    event_mutex.lock();
    while(!event_list.empty())
    {
        event_list.pop();
    }
    auto t = timeout_list.begin();
    while(t != timeout_list.end())
    {
        timeout_list.erase(t);
        t = timeout_list.begin();
    }
    event_mutex.unlock();

    if(stop_callback)
    {
        try
        {
            stop_callback(exit_code);
            stop_callback = nullptr;
        }
        catch(const std::exception&ex)
        {
            std::cerr<< CONSOLE_SYSTEM_ERROR << name << " stop callback exception: " << ex.what() << std::endl;
        }
        catch(...)
        {
            std::cerr<<CONSOLE_SYSTEM_ERROR << name << " stop callback exception" << std::endl;
        }
    }
    return exit_code;
}
int _nw_service::run_one_only(){
    int handler_executed_num = 0;
    event_mutex.lock();

    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    std::map<int64_t,std::vector<nw_timeout_event_ptr>> timed_ready_list;
    try
    {

        for(auto tm = timeout_list.begin(); tm != timeout_list.end(); )
        {

            bool not_deleted = true;
            boost::posix_time::ptime when;
            boost::any when_any;
            if(nw_property_get(tm->second->property_list,"when",when_any)){
            if(nw_property_extract<boost::posix_time::ptime>(when_any,&when)){
            boost::posix_time::time_duration msdiff = now - when;
            int64_t total= msdiff.total_milliseconds();
            if(total>=0)
            {
               auto a = timed_ready_list.find(total);
               if(a != timed_ready_list.end())
                 a->second.push_back(tm->second);
                else {
                 timed_ready_list.insert(std::pair<int64_t,std::vector<nw_timeout_event_ptr>>(total,std::vector<nw_timeout_event_ptr>()));
                 a = timed_ready_list.find(msdiff.total_milliseconds());
                   if(a != timed_ready_list.end()){
                 a->second.push_back(tm->second);
                }
                }
                not_deleted = false;
                timeout_list.erase(tm++);
                break;
            }
            }else std::cout <<CONSOLE_SYSTEM_ERROR <<"unable to extract when property of time event" << std::endl;

            }else std::cout << CONSOLE_SYSTEM_ERROR<<"unable to get when property of time event" << std::endl;
            if(not_deleted)
            tm++;
        }
        if(timed_ready_list.size()>0){
        auto ta = timed_ready_list.end();
        ta--;
        while(ta != timed_ready_list.end()){
            auto tv = ta->second.begin();
            while(tv != ta->second.end()){
             ready_handlers_list.push_back((*tv));
             ta->second.erase(tv);
             tv = ta->second.begin();
            }

            timed_ready_list.erase(ta);
            if(timed_ready_list.size()==0)
            break;
            ta--;
        }
    }
    }
    catch(std::exception&ex)
    {
        std::cerr << CONSOLE_SYSTEM_ERROR<<name << " timeout service exception: "<< ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr <<CONSOLE_SYSTEM_ERROR<< name << " timeout service unknown exception: " << std::endl;
    }
    if(ready_handlers_list.size() == 0){
    while(!event_list.empty())
    {
        nw_event_ptr event = event_list.front();
        ready_handlers_list.push_back(event);
        event_list.pop();
        break;
    }
    }
    event_mutex.unlock();

    auto h = ready_handlers_list.begin();
    while(h != ready_handlers_list.end())
    {
        nw_event_ptr event = (*h);
        ready_handlers_list.erase(h);
        h = ready_handlers_list.begin();
        uint64_t id = event->get_id();
        bool exec = true;
        if(id>0){
         event_mutex.lock();
         exec = (std::find(timeout_clear_request.begin(),timeout_clear_request.end(),id)== timeout_clear_request.end());
         event_mutex.unlock();
        }
        if(!exec)
        continue;
        try
        {
            event->do_action();
            handler_executed_num++;
        }
        catch(const std::exception&ex)
        {
            std::cerr <<CONSOLE_SYSTEM_ERROR << name << " service handler exception: " << ex.what() << std::endl;
            std::cerr << "source : " << event->file << ":" << event->line << std::endl;
        }
        catch(...)
        {
            std::cerr <<CONSOLE_SYSTEM_ERROR << name << " service handler unknown exception: "<< std::endl;
            std::cerr << "source : " << event->file << ":" << event->line << std::endl;
        }

    }

    return handler_executed_num;

}
int _nw_service::run_once()
{

    int handler_executed_num = 0;
    event_mutex.lock();

    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    std::map<int64_t,std::vector<nw_timeout_event_ptr>> timed_ready_list;
    try
    {

        for(auto tm = timeout_list.begin(); tm != timeout_list.end(); )
        {

            bool not_deleted = true;
            boost::posix_time::ptime when;
            boost::any when_any;
            if(nw_property_get(tm->second->property_list,"when",when_any)){
            if(nw_property_extract<boost::posix_time::ptime>(when_any,&when)){
            boost::posix_time::time_duration msdiff = now - when;
            int64_t total= msdiff.total_milliseconds();
            if(total>=0)
            {
               auto a = timed_ready_list.find(total);
               if(a != timed_ready_list.end())
                 a->second.push_back(tm->second);
                else {
                 timed_ready_list.insert(std::pair<int64_t,std::vector<nw_timeout_event_ptr>>(total,std::vector<nw_timeout_event_ptr>()));
                 a = timed_ready_list.find(msdiff.total_milliseconds());
                   if(a != timed_ready_list.end()){
                 a->second.push_back(tm->second);
                }else std::cout <<CONSOLE_SYSTEM_ERROR<< "fatal timed container error" << std::endl;
                }
                not_deleted = false;
                timeout_list.erase(tm++);
            }
            }else std::cout <<CONSOLE_SYSTEM_ERROR <<"unable to extract when property of time event" << std::endl;

            }else std::cout << CONSOLE_SYSTEM_ERROR<<"unable to get when property of time event" << std::endl;
            if(not_deleted)
            tm++;
        }
        if(timed_ready_list.size()>0){
        auto ta = timed_ready_list.end();
        ta--;
        while(ta != timed_ready_list.end()){
            auto tv = ta->second.begin();
            while(tv != ta->second.end()){
             ready_handlers_list.push_back((*tv));
             ta->second.erase(tv);
             tv = ta->second.begin();
            }

            timed_ready_list.erase(ta);
            if(timed_ready_list.size()==0)
            break;
            ta--;
        }
    }
    }
    catch(std::exception&ex)
    {
        std::cerr << CONSOLE_SYSTEM_ERROR<<name << " timeout service exception: "<< ex.what() << std::endl;
    }
    catch(...)
    {
        std::cerr <<CONSOLE_SYSTEM_ERROR<< name << " timeout service unknown exception: " << std::endl;
    }
    while(!event_list.empty())
    {
        nw_event_ptr event = event_list.front();
        ready_handlers_list.push_back(event);
        event_list.pop();
    }
    auto ctr = timeout_clear_request.begin();
    while(ctr != timeout_clear_request.end()){
        timeout_clear_request.erase(ctr);
        ctr = timeout_clear_request.begin();
    }
    event_mutex.unlock();

    auto h = ready_handlers_list.begin();
    while(h != ready_handlers_list.end())
    {
        nw_event_ptr event = (*h);
        ready_handlers_list.erase(h);
        h = ready_handlers_list.begin();
        uint64_t id = event->get_id();
        bool exec = true;
        if(id>0){
         event_mutex.lock();
         exec = (std::find(timeout_clear_request.begin(),timeout_clear_request.end(),id)== timeout_clear_request.end());
         event_mutex.unlock();
        }
        if(!exec)
        continue;
        try
        {
            event->do_action();
            handler_executed_num++;
        }
        catch(const std::exception&ex)
        {
            std::cerr <<CONSOLE_SYSTEM_ERROR << name << " service handler exception: " << ex.what() << std::endl;
            std::cerr << "source : " << event->file << ":" << event->line << std::endl;
        }
        catch(...)
        {
            std::cerr <<CONSOLE_SYSTEM_ERROR << name << " service handler unknown exception: "<< std::endl;
            std::cerr << "source : " << event->file << ":" << event->line << std::endl;
        }

    }
    return handler_executed_num;
}
void _nw_service::stop(int reason,std::function<void(int)>cb)
{
    nw_service_ptr me = shared_from_this();
    if(get_finished())
    {

        NW_SYSTEM_EXEC(([&,reason,cb,me]()->void
        {
            nw_main_clear_timeouts(me);
            exit_code = reason;
            stop_request = true;
            stop_callback = cb;

            event_mutex.lock();
            while(!event_list.empty())
            {
                event_list.pop();
            }
            auto t = timeout_list.begin();
            while(t != timeout_list.end())
            {
                timeout_list.erase(t);
                t = timeout_list.begin();
            }
            event_mutex.unlock();
            stop_request = false;
            set_run_mode(false);

            if(stop_callback)
            {
                try
                {
                    stop_callback(exit_code);
                    stop_callback = nullptr;
                }
                catch(const std::exception&ex)
                {
                    std::cerr <<CONSOLE_SYSTEM_ERROR << "service " << name << " stop callback exception: " << ex.what() << std::endl;
                }
                catch(...)
                {
                    std::cerr << CONSOLE_SYSTEM_ERROR << "service " << name << " stop callback exception" << std::endl;
                }
            }

        }));


        return;
    }

    NW_SERVICE_EXEC(shared_from_this(),([&,reason,cb,me]()->void
    {
        exit_code = reason;
        stop_request = true;
        stop_callback = cb;
        if(!get_run_mode())
        {
            event_mutex.lock();
            while(!event_list.empty())
            {
                event_list.pop();
            }
            auto t = timeout_list.begin();
            while(t != timeout_list.end())
            {
                timeout_list.erase(t);
                t = timeout_list.begin();
            }
            event_mutex.unlock();
            stop_request = false;
            set_run_mode(false);
            if(stop_callback)
            {
                try
                {
                    stop_callback(exit_code);
                    stop_callback = nullptr;
                }
                catch(const std::exception&ex)
                {
                    std::cerr<< CONSOLE_SYSTEM_ERROR << "service " <<  name << " stop callback exception: " << ex.what() << std::endl;
                }
                catch(...)
                {
                    std::cerr << CONSOLE_SYSTEM_ERROR << "service " << name << " stop callback exception" << std::endl;
                }
            }
        }
    }));
}
void _nw_service::exec(std::function<void()>cb,std::string source, int line)
{
    nw_event_ptr evt = nw_exec_event_new(cb,source,line);
    push_event(evt);
}
uint64_t _nw_service::set_timeout(std::function<void()>cb,uint64_t milliseconds,std::string source, int line)
{
    auto self = shared_from_this();
    nw_timeout_event_ptr  ptr= nw_timeout_event_new(self,cb,milliseconds,source,line);
    uint64_t id = ptr->id;
   nw_main_add_timeout(ptr);
    return id;
}
uint64_t _nw_service::set_interval(std::function<bool()>cb,uint64_t milliseconds,std::string source, int line)
{
    nw_interval_event_ptr  ptr= nw_interval_event_new(shared_from(this),cb,milliseconds,source,line);
    uint64_t id = ptr->id;
    nw_main_add_timeout(ptr);
    return id;
}
void _nw_service::set_interval(nw_interval_event_ptr evt)
{
     nw_main_add_timeout(evt);
}

void _nw_service::clear_timeout(uint64_t id)
{
    nw_main_clear_timeout(id);
}
void _nw_service::clear_interval(uint64_t id)
{
    clear_timeout(id);
}
void _nw_service::push_event(nw_event_ptr&evt)
{
    {
        event_mutex.lock();
        event_list.push(evt);
        event_mutex.unlock();
    }
    event_cond.notify_all();
}

void _nw_service::set_persistent(bool b)
{
    std::lock_guard<std::mutex> guard(prop_mutex);
    persistent = b;
}
bool _nw_service::get_persistent()
{
    std::lock_guard<std::mutex> guard(prop_mutex);
    return persistent;
}
void _nw_service::describe(){

std::cout << CONSOLE_SYSTEM_INFO << get_type() << ":" << name << std::endl;
}
void _nw_service::destroy(std::string file, int line)
{
    NW_OBJ_DESTROY_PROTECT
    nw_service_ptr me= shared_from_this();
    if(!nw_system_service_done())
    {
        NW_SYSTEM_EXEC(([&,me]()->void
        {
            me->stop(1,[&,me](int )->void{
                     if(is_reffed())
                nw_object_unref(me);
            });
        }));
    }
    else
    {
        stop(1,[&,me](int )->void
        {
            if(is_reffed())
            nw_object_unref(me);
        });
    }
}
void _nw_service::set_run_mode(bool b)
{
    std::lock_guard<std::mutex> guard(prop_mutex);
    normal_run = b;
}
bool _nw_service::get_run_mode()
{
    std::lock_guard<std::mutex> guard(prop_mutex);
    return normal_run;
}
void _nw_service::set_finished(bool b)
{
    std::lock_guard<std::mutex> guard(prop_mutex);
    finished = b;
}
bool _nw_service::get_finished()
{
    std::lock_guard<std::mutex> guard(prop_mutex);
    return finished;
}
