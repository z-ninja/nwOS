#include <boost/date_time/posix_time/posix_time.hpp>
#include <condition_variable>
#include <boost/bind.hpp>
#include <functional>
#include <mutex>
#include <queue>

#include <nw_system/base.h>
#include <nw_system/nw_object.h>
#include <nw_system/nw_thread.h>
#include <nw_system/nw_service.h>
#include <nw_system/nw_thread_service.h>
#include <nw_system/nw_main.h>
#include <nw_system/nw_assert.h>
#include <nw_system/nw_unique.h>
NW_OBJ_DEF(thread_service,thread_service_ptr,[nw_thread_service])
_nw_thread_service::~_nw_thread_service()
{
    nw_object_ref_decrement(get_type());
}

_nw_thread_service::_nw_thread_service(std::string name):_nw_object(),mythread(nw_thread_new()),myservice(nw_service_new(name+" thread_service"))
{
    NW_ASSERT(nw_property_init<std::function<void(nw_thread_service_ptr,int)>>(property_list,"on-finished",nullptr),"on-finished");
    NW_ASSERT(nw_property_init<std::function<bool(nw_thread_service_ptr,int)>>(property_list,"join-handler",nullptr),"join-handler");
    nw_object_ref_increment(get_type());
    nw_object_unref_real(mythread);
    nw_object_unref_real(myservice);
    nw_thread_run(
    mythread,[&]()->int
    {
        int exit_code = nw_service_run(myservice);
        return exit_code;
    });

    nw_thread_on_status_change(mythread,[&](nw_thread_ptr thr,int status)->void
    {
        if(status == 2){

            property_lock(__FILE__,__LINE__);;
            std::function<void(nw_thread_service_ptr,int)>fn = nullptr;
            NW_ASSERT(nw_property_get_extract<std::function<void(nw_thread_service_ptr,int)>>(property_list,"on-finished",&fn),"on-finished");
            NW_ASSERT(nw_property_set<std::function<void(nw_thread_service_ptr,int)>>(property_list,"on-finished",nullptr),"on-finished");
            property_unlock();
            try{
            if(fn){
                fn(shared_from_this(),nw_thread_get_exit_code(thr));
            }
            }catch(...){
            std::cout << CONSOLE_SYSTEM_ERROR << "error in stopo thread service callbac" << std::endl;
            }
        }
        if(status ==3)
        {
            #ifdef NW_DEBUG
            std::cout << CONSOLE_SYSTEM_INFO<<"thread service thread status finish" << std::endl;
            #endif
            nw_thread_service_ptr me = this->shared_from_this();

           if(is_reffed())
            nw_object_unref_real(me);

        }
    });

}
void _nw_thread_service::stop(std::function<void(nw_thread_service_ptr)>cb)
{

    nw_thread_service_ptr me = shared_from_this();

    property_lock(__FILE__,__LINE__);;
    std::function<bool(nw_thread_service_ptr,int)> fn = nullptr;
    NW_ASSERT(nw_property_get_extract<std::function<bool(nw_thread_service_ptr,int)>>(property_list,"join-handler",&fn),"join-handler");
    NW_ASSERT(nw_property_set<std::function<bool(nw_thread_service_ptr,int)>>(property_list,"join-handler",nullptr),"join-handler");
    property_unlock();

    if(fn){
        bool ret = fn(me,nw_thread_get_status(mythread));
        if(ret)
        return;
    }
    nw_service_stop(myservice,1,[&,me,cb](int exit_code)->void
    {
        //myservice = nullptr;
        nw_thread_stop(
        mythread,[&,me,cb,exit_code](nw_thread_ptr)->void{

            if(cb)
                cb(me);
        });

    });
}
int _nw_thread_service::run_one_only(){
return nw_service_run_one_only(myservice);
}
int _nw_thread_service::run_once(){
return nw_service_run_once(myservice);
}
void _nw_thread_service::exec(std::function<void()>cb,std::string source, int line)
{
    nw_service_exec(myservice,cb,source,line);
}
void _nw_thread_service::destroy(std::string file,int line)
{
    NW_OBJ_DESTROY_PROTECT

    nw_thread_service_ptr mee = shared_from_this();
    stop([&,mee](nw_thread_service_ptr me)->void
    {
        if(is_reffed())
        nw_object_unref_real(me);
    });
}
uint64_t _nw_thread_service::set_timeout(std::function<void()>cb,uint64_t milliseconds,std::string source, int line)
{
    return nw_service_set_timeout(myservice,cb,milliseconds,source,line);
}
uint64_t _nw_thread_service::set_interval(std::function<bool()>cb,uint64_t milliseconds,std::string source, int line)
{
    return nw_service_set_interval(myservice,cb,milliseconds,source,line);
}
void _nw_thread_service::clear_timeout(uint64_t id)
{
    nw_service_clear_timeout(myservice,id);
}
void _nw_thread_service::clear_interval(uint64_t id)
{
    nw_service_clear_interval(myservice,id);
}
void _nw_thread_service::on_finished(std::function<void(nw_thread_service_ptr,int)>cb){
property_lock(__FILE__,__LINE__);;
NW_ASSERT(nw_property_set<std::function<void(nw_thread_service_ptr,int)>>(property_list,"on-finished",cb),"on-finished");
property_unlock();
}
void _nw_thread_service::set_join_handler(std::function<bool(nw_thread_service_ptr,int)>cb){
property_lock(__FILE__,__LINE__);;
NW_ASSERT(nw_property_set<std::function<bool(nw_thread_service_ptr,int)>>(property_list,"join-handler",cb),"join-handler");
property_unlock();
}
/** externals */

NW_PUBLIC nw_thread_service_ptr nw_thread_service_new(std::string name)
{
    nw_thread_service_ptr ptr = nw_thread_service_ptr(new _nw_thread_service(name));
    nw_object_ref(ptr);
    return ptr;
}

NW_PUBLIC bool nw_is_thread_service(nw_object_ptr ptr){
NW_OBJECT_RETURN_BOOL_IS(thread_service,ptr);
}
NW_PUBLIC nw_thread_service_ptr nw_thread_service_from_object(nw_object_ptr ptr){
    if(!ptr)
     NW_NULL_OBJECT_WARNING
    if(nw_is_thread_service(ptr))
        return ((_nw_thread_service*)ptr.get())->shared_from_this();
    return nullptr;
}
NW_PUBLIC nw_object_ptr nw_thread_service_to_object(nw_thread_service_ptr ptr){
    if(!ptr){
    NW_NULL_OBJECT_WARNING
    return nullptr;
    }
return ptr->to_object();
}


NW_PUBLIC void nw_thread_service_destroy(nw_thread_service_ptr ptr,std::string file,int line)
{
    if(ptr)
    {
        NW_SYSTEM_EXEC(([&,ptr,file,line]()->void
        {
            ptr->destroy(file,line);
        }));
    }else NW_NULL_OBJECT_WARNING
}
NW_PUBLIC int nw_thread_service_run_once(nw_thread_service_ptr ptr){
if(ptr)
return ptr->run_once();
else NW_NULL_OBJECT_WARNING
return -1;
}
NW_PUBLIC int nw_thread_service_run_one_only(nw_thread_service_ptr ptr){
if(ptr)
return ptr->run_one_only();
else NW_NULL_OBJECT_WARNING
return -1;
}
NW_PUBLIC void nw_thread_service_stop(nw_thread_service_ptr ptr,std::function<void(nw_thread_service_ptr)>cb){
if(ptr){
    ptr->stop(cb);
}else NW_NULL_OBJECT_WARNING
}
NW_PUBLIC void nw_thread_service_exec(nw_thread_service_ptr ptr,std::function<void()>cb,std::string source, int line){
if(ptr){
  ptr->exec(cb,source,line);
}else NW_NULL_OBJECT_WARNING
}
NW_PUBLIC uint64_t nw_thread_service_set_timeout(nw_thread_service_ptr ptr,std::function<void()>cb,uint64_t milliseconds,std::string source, int line)
{
    if(ptr)
    {

        return ptr->set_timeout(cb,milliseconds,source,line);
    }else NW_NULL_OBJECT_WARNING
    return 0;
}
NW_PUBLIC uint64_t nw_thread_service_set_interval(nw_thread_service_ptr ptr,std::function<bool()>cb,uint64_t milliseconds,std::string source, int line)
{
    if(ptr)
    {

        return ptr->set_interval(cb,milliseconds,source,line);
    }else NW_NULL_OBJECT_WARNING
    return 0;
}
NW_PUBLIC void nw_thread_service_clear_timeout(nw_thread_service_ptr ptr,uint64_t id)
{
    if(ptr)
    {
        ptr->clear_timeout(id);

    }else NW_NULL_OBJECT_WARNING
}
NW_PUBLIC void nw_thread_service_clear_interval(nw_thread_service_ptr ptr,uint64_t id)
{
    if(ptr)
    {
        ptr->clear_interval(id);
    }else NW_NULL_OBJECT_WARNING
}
NW_PUBLIC void nw_thread_service_on_finished(nw_thread_service_ptr ptr,std::function<void(nw_thread_service_ptr,int)>cb){
if(ptr)
    {
        ptr->on_finished(cb);
    }else NW_NULL_OBJECT_WARNING
}
NW_PUBLIC void nw_thread_service_set_join_handler(nw_thread_service_ptr ptr,std::function<bool(nw_thread_service_ptr,int)>cb){
if(ptr)
    {
        ptr->set_join_handler(cb);
    }else NW_NULL_OBJECT_WARNING
}
