#include <boost/date_time/posix_time/posix_time.hpp>
#include <condition_variable>
#include <boost/bind.hpp>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>

#include <nw_system/base.h>
#include <nw_system/nw_object.h>
#include <nw_system/nw_service.h>
#include <nw_system/nw_main.h>
#include <nw_system/nw_thread.h>

#include <nw_system/nw_unique.h>
NW_OBJ_DEF(thread,thread_ptr,[nw_thread])
_nw_thread::~_nw_thread()
{
    nw_object_ref_decrement(get_type());
}

_nw_thread::_nw_thread():_nw_object()
{
    nw_object_ref_increment(get_type());
    std::function<bool(nw_thread_ptr,int)> fn = nullptr;
    std::function<void(nw_thread_ptr,int)> fn_status_change = nullptr;
    nw_property_init<int>(property_list,"status",0);
    nw_property_init<int>(property_list,"exit_code",0);
    nw_property_init<std::thread*>(property_list,"thread",nullptr);
    nw_property_init< std::function<bool(nw_thread_ptr,int)>>(property_list,"join_handler",nullptr);
    nw_property_init< std::function<void(nw_thread_ptr,int)>>(property_list,"thread-status-change",nullptr);
}
int _nw_thread::get_exit_code(){
    std::lock_guard<std::mutex> guard(property_mutex);
    boost::any exit_code_any;

    if(nw_property_get(property_list,"exit_code",exit_code_any))
    {
        int exit_code;
        if(nw_property_extract<int>(exit_code_any,&exit_code))
        {
        return exit_code;
        }else {
        std::cout << "unable to extract property exit_code from thread" << std::endl;
        }

    }
    return 0;
}
int _nw_thread::get_status(){
std::lock_guard<std::mutex> guard(property_mutex);
    boost::any status_any;

    if(nw_property_get(property_list,"status",status_any))
    {
        int status;
        if(nw_property_extract<int>(status_any,&status))
        {
        return status;
        }else {
        std::cout << "unable to extract property status from thread" << std::endl;
        }

    }
    return -1;
}
void _nw_thread::run(std::function<int()>cb)
{
    std::thread* thread = nullptr;
    property_lock(__FILE__,__LINE__);;
    boost::any thread_any;

    if(nw_property_get(property_list,"thread",thread_any))
    {
        if(!nw_property_extract<std::thread*>(thread_any,&thread))
        {

        }
    }
    property_unlock();

    if(thread)
    {
            return;
    }
    property_lock(__FILE__,__LINE__);;
    thread = new std::thread([&,cb](nw_thread_ptr th)->void
    {
        if(th){}
        nw_thread_ptr me = shared_from_this();
        property_lock(__FILE__,__LINE__);;
        std::function<void(nw_thread_ptr,int)> fn_status_change = nullptr;
        boost::any status_change_any;
        if(nw_property_get(property_list,"thread-status-change",status_change_any))
        {
            if(!nw_property_extract<std::function<void(nw_thread_ptr,int)>>(status_change_any,&fn_status_change))
            {

                std::cout << "unable to extract property change status thread" << std::endl;
            }

        }
        nw_property_set<int>(property_list,"status",1);
        property_unlock();
        if(fn_status_change)
        {
            NW_SYSTEM_EXEC(([&,fn_status_change,me]()->void
            {
                fn_status_change(me,1);
            }));
            fn_status_change = nullptr;

        }
        int exit_code = cb();
        property_lock(__FILE__,__LINE__);;
        nw_property_set<int>(property_list,"status",2);
        nw_property_set<int>(property_list,"exit_code",exit_code);
        if(nw_property_get(property_list,"thread-status-change",status_change_any))
        {
            if(!nw_property_extract<std::function<void(nw_thread_ptr,int)>>(status_change_any,&fn_status_change))
            {

                std::cout << "unable to extract property change status thread" << std::endl;
            }

        }
        property_unlock();
        if(fn_status_change)
        {
            NW_SYSTEM_EXEC(([&,fn_status_change,me]()->void
            {
                fn_status_change(me,2);
            }));
            fn_status_change = nullptr;
        }
    },shared_from_this());


    nw_property_set<std::thread*>(property_list,"thread",thread);
    property_unlock();
}
void _nw_thread::stop(std::function<void(nw_thread_ptr)>cb)
{
    nw_thread_ptr me = shared_from_this();
    NW_SYSTEM_EXEC(([&,me,cb]()->void
    {
        me->join();
        if(cb)
            cb(me);
    }));
}
int _nw_thread::join()
{


    nw_thread_ptr me = shared_from_this();
    int status = 0;
    std::thread* thread = nullptr;
    property_lock(__FILE__,__LINE__);;
    boost::any thread_any;
    boost:: any thread_status;
    boost:: any thread_join_handler;
    std::function<bool(nw_thread_ptr,int)> fn = nullptr;
    if(nw_property_get(property_list,"thread",thread_any))
    {
        if(!nw_property_extract<std::thread*>(thread_any,&thread))
        {

            std::cout <<CONSOLE_SYSTEM_ERROR<< "unable to extract property thread" << std::endl;
        }

    }else std::cout << CONSOLE_SYSTEM_ERROR<<"unable to get thread property" << std::endl;
    if(nw_property_get(property_list,"status",thread_status))
    {
        if(!nw_property_extract<int>(thread_status,&status))
        {

            std::cout << CONSOLE_SYSTEM_ERROR<<"unable to extract property thread" << std::endl;
        }

    }
    if(nw_property_get(property_list,"join_handler",thread_join_handler))
    {
        if(!nw_property_extract<std::function<bool(nw_thread_ptr,int)>>(thread_join_handler,&fn))
        {

            std::cout << CONSOLE_SYSTEM_ERROR<<"unable to extract property thread" << std::endl;
        }

    }
    property_unlock();

    if(fn)
    {

        bool fn_ret = fn(me,status);
        property_lock(__FILE__,__LINE__);;
        fn = nullptr;
        thread_join_handler = fn;
        nw_property_set<std::function<bool(nw_thread_ptr,int)>>(property_list,"join_handler",nullptr);
        property_unlock();
        if(!fn_ret)
        {
            return status;
        }
    }


    if(thread&&thread->joinable())
    {
        thread->join();
        delete thread;
        thread = nullptr;
        thread_any = thread;
        property_lock(__FILE__,__LINE__);;
        nw_property_set<std::thread*>(property_list,"thread",nullptr);
        nw_property_set<int>(property_list,"status",3);
        std::function<void(nw_thread_ptr,int)> fn_status_change = nullptr;
        boost::any status_change_any;
        if(nw_property_get(property_list,"thread-status-change",status_change_any))
        {
            if(!nw_property_extract<std::function<void(nw_thread_ptr,int)>>(status_change_any,&fn_status_change))
            {

                std::cout <<CONSOLE_SYSTEM_ERROR<< "unable to extract property change status thread" << std::endl;
            }

        }



        nw_property_set<std::function<void(nw_thread_ptr,int)>>(property_list,"thread-status-change",nullptr);
        property_unlock();
         if(fn_status_change){
            if(nw_system_service_done()){

            }else {
            NW_SYSTEM_EXEC(([&,fn_status_change,me]()->void
            {
                fn_status_change(me,3);
            }));
            }
            fn_status_change = nullptr;
        }

    }
    return status;
}
void _nw_thread::destroy(std::string file,int line)
{
    NW_OBJ_DESTROY_PROTECT
    if(get_status() == 3){
            if(is_reffed())
        nw_object_unref(shared_from_this());
    }else
    stop([&](nw_thread_ptr)->void{
         if(is_reffed())
         nw_object_unref_real(shared_from_this());
        });

}
void nw_thread_join(nw_thread_ptr ptr)
{
    if(ptr)
    {
        ptr->join();
    }else NW_NULL_OBJECT_WARNING
}
/** exports */

NW_PUBLIC nw_thread_ptr nw_thread_new()
{
    nw_thread_ptr ptr = nw_thread_ptr(new _nw_thread());
    nw_object_ref(ptr);
    return ptr;
}
NW_PUBLIC  bool nw_is_thread(nw_object_ptr ptr)
{
NW_OBJECT_RETURN_BOOL_IS(thread,ptr);

}

NW_PUBLIC nw_thread_ptr nw_thread_from_object(nw_object_ptr ptr){
    if(!ptr)
NW_NULL_OBJECT_WARNING
if(nw_is_thread(ptr))
        return ((_nw_thread*)ptr.get())->shared_from_this();
    return nullptr;
}
NW_PUBLIC nw_object_ptr nw_thread_to_object(nw_thread_ptr ptr){

 if(!ptr){
    NW_NULL_OBJECT_WARNING
    return nullptr;
 }
return ptr->to_object();
}

NW_PUBLIC void nw_thread_run(nw_thread_ptr ptr,std::function<int()>cb)
{
    if(ptr)
    {
        ptr->run(cb);
    }else NW_NULL_OBJECT_WARNING
}
NW_PUBLIC int nw_thread_get_exit_code(nw_thread_ptr ptr)
{
    if(ptr)
    {
      return  ptr->get_exit_code();
    }else NW_NULL_OBJECT_WARNING
    return -1;
}
NW_PUBLIC int nw_thread_get_status(nw_thread_ptr ptr)
{
    if(ptr)
    {
      return  ptr->get_status();
    }else NW_NULL_OBJECT_WARNING
    return -1;
}
NW_PUBLIC void nw_thread_stop(nw_thread_ptr ptr,std::function<void(nw_thread_ptr)>cb)
{
    if(ptr)
    {
        ptr->stop(cb);
    }else NW_NULL_OBJECT_WARNING
}

NW_PUBLIC bool nw_thread_set_join_handler(nw_thread_ptr ptr,std::function<bool(nw_thread_ptr,int)>cb)
{
    if(ptr)
    {
        std::lock_guard<std::mutex> guard(ptr->property_mutex);
        return nw_property_set<std::function<bool(nw_thread_ptr,int)>>(ptr->property_list,"join_handler",cb);
    }else NW_NULL_OBJECT_WARNING
    return false;
}
NW_PUBLIC void nw_thread_destroy(nw_thread_ptr ptr,std::string file,int line)
{
    if(ptr)
    {
        ptr->destroy(file,line);
    }else NW_NULL_OBJECT_WARNING
}
NW_PUBLIC bool nw_thread_on_status_change(nw_thread_ptr ptr,std::function<void(nw_thread_ptr,int)> cb)
{

    if(ptr)
    {
        std::lock_guard<std::mutex> guard(ptr->property_mutex);
        return nw_property_set<std::function<void(nw_thread_ptr,int)>>(ptr->property_list,"thread-status-change",cb);
    }else NW_NULL_OBJECT_WARNING
    return false;
}
