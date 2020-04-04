#include <nw_system/nw_assert.h>
#include <condition_variable>
#include <boost/bind.hpp>
#include <functional>
#ifndef NW_SYSTEM_NO_GTK
#include <gtk/gtk.h>
#endif
#include <mutex>
#include <queue>



#include <nw_system/base.h>
#include <nw_system/nw_object.h>
#include <nw_system/nw_event.h>
#include <nw_system/nw_service.h>
#include <nw_system/nw_main.h>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#ifndef NW_SYSTEM_USE_IO_SERICE
typedef boost::asio::io_context asio_service;
#else
typedef boost::asio::io_service asio_service;
#endif // NW_WINDOWS
class timer_service : public boost::enable_shared_from_this<timer_service>
{
private:
    asio_service ios;
    boost::asio::steady_timer timer;
    std::map<uint64_t,nw_timeout_event_ptr> timeout_list;
    std::thread thread;
    std::vector<uint64_t> timeout_clear_request;
    std::mutex timeout_clear_request_mutex;
    boost::posix_time::ptime next_complete_time;
public:
    typedef boost::shared_ptr<timer_service> timer_service_ptr;
    timer_service():ios(),timer(ios),timeout_list(),thread([&]()->void
    {
        run();
    }),timeout_clear_request(),timeout_clear_request_mutex(),next_complete_time()
    {

    }
    ~timer_service()
    {

    }
    void stop()
    {

        ios.stop();
        timer.cancel();
        while(ios.poll_one()>0);
        thread.join();
        auto e = timeout_list.begin();
        while(e != timeout_list.end())
        {
            timeout_list.erase(e);
            e = timeout_list.begin();
        }
    }
    void set_timeout(nw_timeout_event_ptr tm)
    {
        auto self = shared_from_this();
        #ifdef NW_SYSTEM_USE_IO_SERICE
        ios.post(boost::bind(&timer_service::set_timeout,this,self,tm));
        #else
        boost::asio::post(ios,boost::bind(&timer_service::set_timeout,this,self,tm));
        #endif // NW_SYSTEM_USE_IO_SERICE

    }
    void clear_timeout(uint64_t id)
    {

        auto self = shared_from_this();
        timeout_clear_request_mutex.lock();
        timeout_clear_request.push_back(id);
        timeout_clear_request_mutex.unlock();
        #ifdef NW_SYSTEM_USE_IO_SERICE
        ios.post(
        #else
        boost::asio::post(ios,
        #endif
                          [&,self,id]()->void
        {
            auto e = timeout_list.find(id);
            if(e != timeout_list.end())
            {
                timeout_list.erase(e);
                recalculate();
            }
        });
    }
    void clear_timeouts(nw_service_ptr svr)
    {
        auto self = shared_from_this();
        #ifdef NW_SYSTEM_USE_IO_SERICE
        ios.post(
        #else
        boost::asio::post(ios,
        #endif
        [&,self,svr]()->void
        {
            for(auto begin = timeout_list.end(); begin != timeout_list.end();)
            {
                if(begin->second->service->id == svr->id)
                {
                    timeout_list.erase(begin++);
                }
                else
                {
                    ++begin;
                }
            }
        });
    }
private:
    void set_timeout(boost::shared_ptr<timer_service> self,nw_timeout_event_ptr tm)
    {
        timeout_list.insert(std::pair<uint64_t,nw_timeout_event_ptr>(tm->get_id(),tm));

        if(timeout_list.size()==1){
        boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::ptime when;
        NW_ASSERT(nw_property_get_extract<boost::posix_time::ptime>(tm->property_list,"when",&when),"when property");
        boost::posix_time::time_duration msdiff = now - when;
        int64_t total= msdiff.total_milliseconds();
        if(total<0){
        int64_t next_tm  = total * -1;
        next_complete_time = when;
        timer.cancel();
        timer.expires_from_now(std::chrono::milliseconds(next_tm));
        timer.async_wait([&,self](const boost::system::error_code&e)->void
        {
            if (!e)
            {
                recalculate();
            }
        });
        }else recalculate();
        } else {

        boost::posix_time::ptime when;
        NW_ASSERT(nw_property_get_extract<boost::posix_time::ptime>(tm->property_list,"when",&when),"when property");
        boost::posix_time::time_duration msdiff = next_complete_time - when;
        int64_t total= msdiff.total_milliseconds();
        if(total > 0){
        boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::time_duration msdiff1 = now - when;
        total = msdiff1.total_milliseconds();
        if(total <0){
           next_complete_time = when;
           int64_t next_tm = total * -1;
        if(next_tm >= 50)
        {
            next_tm -= 50;
        }

        timer.cancel();
        timer.expires_from_now(std::chrono::milliseconds(next_tm));
        timer.async_wait([&,self](const boost::system::error_code&e)->void
        {
            if (!e)
            {
                recalculate();
            }
        });
        }else {
        recalculate();
        }
        }else{
        recalculate();
        }
        }
    }
    void run()
    {
        try
        {
            #ifdef NW_SYSTEM_USE_IO_SERICE
            asio_service::work work(ios);
            #else
            boost::asio::executor_work_guard<asio_service::executor_type> work(boost::asio::make_work_guard(ios));
            #endif // NW_SYSTEM_USE_IO_SERICE
            ios.run();
        }
        catch(std::exception&ex)
        {
            std::cerr << CONSOLE_SYSTEM_ERROR << " timeout service exception: "<< ex.what() << std::endl;
        }
    }
    void recalculate()
    {
        int64_t next_tm = 60000;
        auto self = shared_from_this();
        boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
        std::vector<uint64_t> clear_requests;
        timeout_clear_request_mutex.lock();

        for( auto bg = timeout_clear_request.begin(); bg != timeout_clear_request.end();bg++)
        {
            clear_requests.push_back(*bg);
        }
        timeout_clear_request_mutex.unlock();
        for(auto tm = timeout_list.begin(); tm != timeout_list.end();)
        {

            nw_timeout_event_ptr evt = tm->second;
            auto rtm = std::find(clear_requests.begin(),clear_requests.end(),evt->id);
            if(rtm != clear_requests.end())
            {
                clear_requests.erase(rtm);
                timeout_list.erase(tm++);
                timeout_clear_request_mutex.lock();
                rtm = std::find(timeout_clear_request.begin(),timeout_clear_request.end(),evt->id);
                if(rtm != timeout_clear_request.end())
                {
                    timeout_clear_request.erase(rtm);
                }
                timeout_clear_request_mutex.unlock();
            }
            else
            {
                boost::posix_time::ptime when;
                boost::any when_any;
                if(nw_property_get(evt->property_list,"when",when_any))
                {
                    if(nw_property_extract<boost::posix_time::ptime>(when_any,&when))
                    {
                        boost::posix_time::time_duration msdiff = now - when;
                        int64_t total= msdiff.total_milliseconds();
                        if(total>=-10)
                        {
                            nw_service_exec(evt->service,[&,self,evt]()->void
                            {
                                timeout_clear_request_mutex.lock();
                                auto rtm2 = std::find(timeout_clear_request.begin(),timeout_clear_request.end(),evt->id);
                                if(rtm2 != timeout_clear_request.end())
                                {

                                    timeout_clear_request.erase(rtm2);
                                    timeout_clear_request_mutex.unlock();
                                    return;
                                }
                                timeout_clear_request_mutex.unlock();
                                try
                                {
                                    evt->do_action();
                                }
                                catch(std::exception&ex)
                                {
                                    std::cerr << CONSOLE_SYSTEM_ERROR << " timeout event exception: "<< ex.what()
                                              << evt->file << ":"<<evt->line << std::endl;
                                }
                            },evt->file,evt->line);
                            timeout_list.erase(tm++);
                        }
                        else
                        {
                            total = total * -1;
                            if(total < next_tm)
                            {
                                next_complete_time = when;
                                next_tm = total;
                            }
                            ++tm;
                        }
                    }
                    else
                    {
                        ++tm;
                    }
                }
                else
                {
                    ++tm;
                }
            }
        }

        if(next_tm >= 10)
        {
            next_tm -= 10;
        }

        timer.cancel();
        timer.expires_from_now(std::chrono::milliseconds(next_tm));
        timer.async_wait([&,self](const boost::system::error_code&e)->void
        {
            if (!e)
            {
                recalculate();
            }

        });
    }

};

typedef timer_service::timer_service_ptr timer_service_ptr;
timer_service_ptr timer_service_new()
{
    timer_service_ptr ptr = timer_service_ptr(new timer_service());
    return ptr;
}
timer_service_ptr timeout_service = nullptr;

void nw_set_thread_id(std::thread::id id);




nw_service_ptr system_service= nullptr;
std::mutex system_prop_mutex;
bool system_done = false;
void nw_system_set_done(bool b)
{
    std::lock_guard<std::mutex> guard(system_prop_mutex);
    system_done = b;
}
#ifndef NW_SYSTEM_NO_GTK
gboolean nw_system_idle_func(gpointer)
{
    nw_system_service_run_once();
    return true;
}
#endif
int exit_reason = 0;


/** exports */

void nw_main_add_timeout(nw_timeout_event_ptr tm)
{
    if(timeout_service != nullptr)
    {
        timeout_service->set_timeout(tm);
    }
}
void nw_main_clear_timeout(uint64_t id)
{
    if(timeout_service != nullptr)
    {
        timeout_service->clear_timeout(id);
    }
}
void nw_main_clear_timeouts(nw_service_ptr svr)
{
    if(timeout_service != nullptr)
    {
        timeout_service->clear_timeouts(svr);
    }
}

NW_PUBLIC int nw_system_service_run_once()
{
    return nw_service_run_once(system_service);
}

NW_PUBLIC int nw_system_init(int &argc,char **&argv)
{


    #ifndef NW_SYSTEM_NO_GTK
    gtk_init(&argc,&argv);
    #endif
    if(!system_service)
    {
        system_service = nw_service_new("main");
        nw_object_unref_real(system_service->to_object());
    }
    if(!timeout_service)
    {
        timeout_service = timer_service_new();
    }
    return 0;
}
NW_PUBLIC int nw_system_service_run(int interval)
{
    #ifndef NW_SYSTEM_NO_GTK
    g_timeout_add(interval,nw_system_idle_func,nullptr);
    #endif
    nw_set_thread_id(std::this_thread::get_id());
    #ifndef NW_SYSTEM_NO_GTK
    gtk_main();
    #else
    nw_service_run(system_service);
    #endif
    std::cout <<CONSOLE_SYSTEM_INFO << "nw_system_done" << std::endl;
    nw_system_set_done(true);
    nw_object_clean_up(__FILE__,__LINE__);
    system_service = nullptr;
    return exit_reason;
}

void exit_system(int reason)
{

    nw_object_set_exit_handler([&,reason]()->void
    {
        nw_service_stop(system_service,reason,[&](int r)
        {
            exit_reason = r;
            #ifndef NW_SYSTEM_NO_GTK
            gtk_main_quit();
            #endif
            if(timeout_service != nullptr)
            {
                timeout_service->stop();
                timeout_service = nullptr;
            }
        });
    });

    nw_object_destroy_all();
}
NW_PUBLIC void nw_system_service_quit(int reason)
{
    NW_SYSTEM_EXEC(boost::bind(&exit_system,reason));
}
NW_PUBLIC void nw_system_service_exec(std::function<void()>cb, std::string source, int line)
{
    nw_service_exec(system_service,cb,source,line);
}
NW_PUBLIC uint64_t nw_system_service_timeout(std::function<void()>cb,uint64_t milliseconds,std::string source, int line)
{
    return nw_service_set_timeout(system_service,cb,milliseconds,source,line);
}
NW_PUBLIC uint64_t nw_system_service_interval(std::function<bool()>cb,uint64_t milliseconds,std::string source, int line)
{
    return nw_service_set_interval(system_service,cb,milliseconds,source,line);
}
NW_PUBLIC void nw_system_service_timeout_clear(uint64_t id)
{
    nw_service_clear_timeout(system_service,id);
}
NW_PUBLIC void nw_system_service_interval_clear(uint64_t id)
{
    nw_service_clear_interval(system_service,id);
}
NW_PUBLIC void nw_system_service_push_event(nw_event_ptr&evt)
{
    nw_service_push_event(system_service,evt);
}
NW_PUBLIC bool nw_system_service_done()
{
    std::lock_guard<std::mutex> guard(system_prop_mutex);
    return system_done;
}


