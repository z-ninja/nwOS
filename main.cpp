//g++ -std=c++11 -Wall -fexceptions -g  -c main.cpp -o main.o
//g++  -o loop_test main.o  -lpthread -latomic
#include <condition_variable>
#include <functional>
#include <algorithm>
#include <iostream>
#include <cassert>
#include <future>
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>
#include <vector>
#include <mutex>
#include <queue>
#include <map>

class Object;
class EventLoop;
class Thread_poll;
class TimerService;

EventLoop*g_main_loop = nullptr;
TimerService*g_timer_service  = nullptr;
Thread_poll*g_thread_poll = nullptr;
void object_check_out(Object*,std::function<void()>a_cb = nullptr);
std::mutex g_object_list_mutex;
std::vector<Object*> g_objects_list;
std::vector<Object*> g_subscribe_objects_list;/// for object which needs to receive event when program is about to quit...
std::atomic_int ref_counter(0);
std::atomic_int ref_aloc(0);
std::atomic_int ref_dealoc(0);
class Object
{
protected:
    std::atomic_int m_ref_c; /// reference count. >=1  is in g_objects_list
    std::atomic_bool m_sub_l; /// indicator if object in g_subscribe_objects_list
    std::atomic_bool m_des_p; /// indicator if object is in destruction mode
public:

    Object():m_ref_c(0),m_sub_l(false),m_des_p(false)
    {
        ref_counter++;
        ref_aloc++;
    }
    virtual~Object()
    {
        ref_counter--;
        ref_dealoc++;
    }
    virtual void destroy(std::function<void()>a_cb)
    {

        a_cb();
    }
    std::atomic_int& use_count()
    {
        return m_ref_c;
    }
    /// add objct to g_subscribe_objects_list
    void subscribe()
    {
        if(!m_sub_l)
        {
            m_sub_l = true;
            std::lock_guard<std::mutex> l_guard(g_object_list_mutex);
            g_subscribe_objects_list.push_back(this);

        }
    }
    void check_in()
    {
        if(m_des_p)
        {
            /** we are in destruction mode, no need for referencing, this should not happen in organized code,
             maybe to include this step only in debug version for debugging only to get
             better performance in release version? */
            std::cout << "WARNING: object check in call after destruction mode" << std::endl;
            return;
        }
        m_ref_c++;
        ///std::cout << "m_ref_c++: " << m_ref_c<< std::endl;

        if(m_ref_c == 1)
        {
            std::lock_guard<std::mutex> l_guard(g_object_list_mutex);
            g_objects_list.push_back(this);
        }
    }
    bool check_out(std::function<void()>a_cb = nullptr)
    {
        ///std::cout << "checkout" << std::endl;
        m_ref_c--;
        // std::cout << "m_ref_c--: " << m_ref_c<< std::endl;
        /// moving from >=1 to 0 means object will be destroyed
        if(m_ref_c == 0)
        {
            m_des_p = true;/// set m_des_p to true since destroy can call callback in asynchronous way, from other thread.. etc..

            if(m_sub_l)
            {
                m_sub_l = false;
                auto a = std::find(g_subscribe_objects_list.begin(),g_subscribe_objects_list.end(),this);
                if(a != g_subscribe_objects_list.end())
                {
                    g_subscribe_objects_list.erase(a);
                }
            }
            return true;
        }
        /// < 0 means object did not ever been on g_objects_list, manual deletion is needed in this case
        else if(m_ref_c <0&&!m_des_p)
        {
            m_des_p = true;
            if(m_sub_l)
            {
                ///  but maybe is in g_subscribe_objects_list
                m_sub_l = false;
                auto a = std::find(g_subscribe_objects_list.begin(),g_subscribe_objects_list.end(),this);
                if(a != g_subscribe_objects_list.end())
                {
                    g_subscribe_objects_list.erase(a);
                }
            }
            return true;
        }
        return false;
    }

};
class Event
{
public:
    Event()
    {
        ref_counter++;
        ref_aloc++;
        //std::cout << "event new:" << ref_counter << ":" << ref_aloc << std::endl;
    }
    virtual~Event()
    {
        ref_counter--;
        ref_dealoc++;
        //std::cout << "event gone:" << ref_counter << ":" << ref_dealoc << std::endl;
    }
    virtual void action() {}
};
class ExecEvent : public Event
{
public:
    std::function<void()>m_cb;
    ExecEvent(std::function<void()>a_cb):Event(),m_cb(a_cb) {}
    virtual~ExecEvent() {}
    virtual void action()
    {
        if(m_cb != nullptr)
        {
            m_cb();
        }
    }
};
class EventLoop : public Object
{
protected:

    std::atomic_bool m_ready;
    std::atomic_bool m_running;
    std::queue<Event*> m_event_list;
    std::mutex m_event_mutex;
    std::condition_variable m_event_cond;

public:
    std::atomic_bool& is_ready()
    {
        return m_ready;
    }
    std::atomic_bool& is_running()
    {
        return m_running;
    }
    EventLoop():Object(),m_ready(true),m_running(false),m_event_list(),m_event_mutex(),m_event_cond()
    {

    }
    virtual~EventLoop()
    {


        if(!m_running)
        {
            std::lock_guard<std::mutex>l_guard(m_event_mutex);

            while(!m_event_list.empty())
            {
                Event* l_event = m_event_list.front();
                delete l_event;
                m_event_list.pop();
            }
        }

    }

    /// in case where we need to integrate with other frameworks this can help to become flexible
    virtual void stop()
    {

        m_running = false;
        m_event_cond.notify_all();
    }
    virtual size_t run_once()
    {
        std::vector<Event*> l_ready_events;
        m_event_mutex.lock();
        while(!m_event_list.empty())
        {
            Event* l_event = m_event_list.front();
            l_ready_events.push_back(l_event);
            m_event_list.pop();
        }
        m_event_mutex.unlock();
        size_t handlers_called = 0;
        auto l_e = l_ready_events.begin();
        while(l_e != l_ready_events.end())
        {
            handlers_called++;
            (*l_e)->action();
            delete (*l_e);
            l_ready_events.erase(l_e);
            l_e = l_ready_events.begin();
        }
        return handlers_called;
    }
    virtual void run()
    {
        if(m_running)
        {
            std::cout << "Warnning: Event loop method \"run\" duplicated call" << std::endl;
            return;
        }
        m_ready = true;
        m_running = true;
        while(m_running)
        {
            /// using local container to avoid dead lock in event action call
            // std::cout << "loop start" << std::endl;
            std::vector<Event*> l_ready_events;
            std::unique_lock<std::mutex>l_lock(m_event_mutex);
            /// wait until we get at least one event in m_event_list
            /// P.S this call will not block silently unless we link thread library (-lpthread not sure if there are alternative library)
            /// in project,otherwise will act as a loop and draw CPU
            m_event_cond.wait(l_lock

                              ,[&]()->bool
            {
                /// while this callback is called, lock is locked so it is safe to not use lock here
                return !m_running || !m_event_list.empty();
            });
            /// collect all events
            while(!m_event_list.empty())
            {
                Event* l_event = m_event_list.front();
                l_ready_events.push_back(l_event);
                m_event_list.pop();
            }
            /// unlock the lock, process each event and destroy them.
            /// NOTE: if handler increment event ref counter, it will stay alive at this point
            l_lock.unlock();
            auto l_e = l_ready_events.begin();
            while(l_e != l_ready_events.end())
            {
                try
                {
                    (*l_e)->action();
                }
                catch(std::exception&ex)
                {
                    std::cout << "event handler exception: " << ex.what() << std::endl;
                }
                catch(...)
                {
                    std::cout << "event handler unknown exception: " << std::endl;
                }
                delete (*l_e);
                l_ready_events.erase(l_e);
                l_e = l_ready_events.begin();
            }
            // std::cout << "loop end" << std::endl;
        }/// end of loop
        m_ready = false;
        std::vector<Event*> l_evt_list;
        {
            /// get read of all left events since we are closing the loop
            m_event_mutex.lock();
            while(!m_event_list.empty())
            {
                Event* l_event = m_event_list.front();
                m_event_list.pop();
                l_evt_list.push_back(l_event);
                //object_check_out(l_event);
            }
            m_event_mutex.unlock();
        }

        {
            auto l_m = l_evt_list.begin();
            while(l_m != l_evt_list.end())
            {
                Event* l_event = (*l_m);
                delete l_event;
                l_evt_list.erase(l_m);
                l_m = l_evt_list.begin();
            }
        }

        ///  not sure if we should allow this here to allow event push in case we want to restart,
        ///  I am familiar in multi threading this can make memory leaks in event is pushed after m_running = false, will see
        /// m_ready = true;
    }
    void push_event(Event*a_evt)
    {
        /// if event is ready/running, we can push event, maybe return a bool will help in some scenarios, will see...
        if(m_ready)
        {
            {
                std::lock_guard<std::mutex> l_lock(m_event_mutex);
                m_event_list.push(a_evt);
            }
            /// notify condition variable that list is updated
            m_event_cond.notify_all();
        }
        else std::cout << "event loop event push declined" << std::endl;
    }
    void post(std::function<void()>a_cb)
    {
        /// post execute handler event to be called in thread from where run is called
        push_event(new ExecEvent(a_cb));
    }

    void destroy(std::function<void()>a_cb)
    {
        if(m_running)/// if we are running
        {
            post([&,a_cb]()->void
            {
                m_ready = false;
                m_running = false;
                if(a_cb != nullptr)
                {
                    a_cb();
                }
            });
        }
        else
        {
            if(a_cb != nullptr)
            {
                a_cb();
            }
        }
    }
};
class Thread : public EventLoop
{
protected:
    std::promise<void> m_promise;
    std::thread m_thread;
public:
    Thread():EventLoop(),m_promise(),m_thread([&]
    {
        m_event_mutex.lock();
        m_promise.set_value();
        m_event_mutex.unlock();
        run();
    })
    {
        m_event_mutex.lock();
        std::future<void> l_future = m_promise.get_future();
        m_event_mutex.unlock();
        l_future.wait();
    }
    virtual~Thread()
    {
        if(m_thread.joinable())
        {
            m_thread.join();/// when joining the thread very rare friezes the process, not sure why
        }

    }
    void destroy(std::function<void()>a_cb)
    {
        if(is_running())
        {
            EventLoop::destroy([&,a_cb]()->void
            {
                g_main_loop->post([&,a_cb]()->void
                {
                    if(a_cb != nullptr)
                    {
                        a_cb();
                    }
                });
            });
        }
        else
        {
            g_main_loop->post([&,a_cb]()->void
            {

                if(a_cb != nullptr)
                {
                    a_cb();
                }
            });
        }
    }
};
class Thread_poll : public Object
{
protected:
    std::mutex m_thread_list_mutex;
    std::map<Thread*,bool> m_thread_list;
    size_t m_free_threads;
public:

    Thread_poll():Object(),m_thread_list_mutex(),m_thread_list(),m_free_threads(0)
    {
    }
    virtual~Thread_poll()
    {
    }
    Thread*get()
    {
        std::lock_guard<std::mutex>l_guard(m_thread_list_mutex);
        if(m_free_threads <= 0)
        {
            Thread*l_th = new Thread();
            l_th->check_in();
            m_thread_list.insert(std::make_pair(l_th,true));
            return l_th;
        }
        else
        {
            for(auto f_th = m_thread_list.begin(); f_th != m_thread_list.end(); f_th++)
            {
                if(!f_th->second)
                {
                    m_free_threads--;
                    f_th->second = true;
                    return f_th->first;
                }
            }
        }
        std::cout << "WARNING: not found free thread" << std::endl;
        /// should never happen
        return nullptr;
    }
    void put(Thread*a_th)
    {
        std::lock_guard<std::mutex>l_guard(m_thread_list_mutex);
        auto l_th = m_thread_list.find(a_th);
        if(l_th != m_thread_list.end())
        {
            l_th->second = false;
            m_free_threads++;
        }
    }
    void add(Thread*a_th,bool a_in_use)
    {
        std::lock_guard<std::mutex>l_guard(m_thread_list_mutex);
        auto l_th = m_thread_list.find(a_th);
        if(l_th == m_thread_list.end())
        {
            a_th->check_in();
            if(!a_in_use)
            {
                m_free_threads++;
            }
            m_thread_list.insert(std::make_pair(a_th,a_in_use));
        }
    }
    void remove(Thread*a_th)
    {
        std::lock_guard<std::mutex>l_guard(m_thread_list_mutex);
        auto l_th = m_thread_list.find(a_th);
        if(l_th != m_thread_list.end())
        {
            if(!l_th->second)
            {
                m_free_threads--;
            }
            object_check_out(l_th->first);
            m_thread_list.erase(l_th);
        }
    }
    void destroy(std::function<void()>a_cb)
    {
        /// std::cout << "thread poll destroy start" << std::endl;
        std::map<Thread*,bool> l_thread_list;
        {
            std::lock_guard<std::mutex>l_guard(m_thread_list_mutex);
            auto l_th = m_thread_list.begin();
            while(l_th != m_thread_list.end())
            {
                l_thread_list.insert(std::pair<Thread*,bool>(l_th->first,l_th->second));
                m_thread_list.erase(l_th);
                l_th = m_thread_list.begin();
                //   break;
            }
        }

        {

            auto l_th = l_thread_list.begin();
            while(l_th != l_thread_list.end())
            {
                Thread*l_oth = l_th->first;
                if(!l_th->second)
                {
                    m_free_threads--;
                }
                l_thread_list.erase(l_th);
                l_th = l_thread_list.begin();

                object_check_out(l_oth);/*,[&,a_cb]()->void
                {
                   // destroy(a_cb);
                });*/
                // return;
            }
            if(a_cb != nullptr)
            {
                a_cb();
            }
        }
    }
};
class Timeout : public Event
{
public:
    std::function<void()>m_cb;
    EventLoop*m_event_loop;
    std::chrono::steady_clock::time_point m_deadline;
    int64_t m_repeat;
    Timeout(EventLoop*a_el,std::function<void()>a_cb, int64_t a_duration,bool a_repeat = false):Event(),m_cb(a_cb),m_event_loop(a_el),m_deadline(),m_repeat(0)
    {
        if(m_event_loop != nullptr)
        {
            m_event_loop->check_in();
        }
        if(a_repeat)
        {
            m_repeat = a_duration;
        }
        auto now = std::chrono::steady_clock::now();
        m_deadline = now+std::chrono::milliseconds(a_duration);

    }


    virtual~Timeout()
    {
        if(m_event_loop != nullptr)
        {
            object_check_out(m_event_loop);
        }
    }
    virtual void action()
    {

        if(m_cb != nullptr)
        {
            if(m_event_loop != nullptr)
            {
                m_event_loop->post(m_cb);
            }
            else
            {
                g_main_loop->post(m_cb);
            }
        }
    }

};
//// boost::posix_time::time_duration msdiff = now - (last_relocation_sent_time+boost::posix_time::milliseconds(time_out));
//      int64_t total= msdiff.total_milliseconds();

struct
{
    bool operator()(Timeout*a, Timeout*b) const
    {
        return a->m_deadline < b->m_deadline;
    }
} timeout_sorter;
class TimerService : public Object
{
protected:
    std::atomic_bool m_ready;
    std::atomic_bool m_running;
    std::queue<Timeout*> m_event_list;
    std::vector<Timeout*> m_timeout_list;
    std::mutex m_event_mutex;
    std::condition_variable m_event_cond;
    Timeout*m_best_timer;
    std::promise<void> m_promise;
    std::thread m_thread;

public:
    TimerService():Object(),m_ready(true),m_running(false),m_event_list(),m_timeout_list(),m_event_mutex(),
        m_event_cond(),m_best_timer(nullptr),m_promise(),m_thread([&]
    {

        run();
        // std::cout << "timeout thread exit" << std::endl;
    })
    {
        m_event_mutex.lock();
        std::future<void> l_future = m_promise.get_future();
        m_event_mutex.unlock();
    }
    virtual void run()
    {
        m_running = true;
        m_ready = true;
        m_event_mutex.lock();
        m_promise.set_value();

        while(!m_event_list.empty())
        {
            Timeout*l_timeout = m_event_list.front();
            m_timeout_list.push_back(l_timeout);
            m_event_list.pop();
        }
        std::sort(m_timeout_list.begin(),m_timeout_list.end(),timeout_sorter);
        auto l_bt = m_timeout_list.begin();
        if(l_bt != m_timeout_list.end())
        {
            m_best_timer = (*l_bt);

        }
        m_event_mutex.unlock();
        while(m_running)
        {
            m_event_mutex.lock();
            if((!m_event_list.empty()))
            {
                while(!m_event_list.empty())
                {
                    Timeout*l_timeout = m_event_list.front();
                    m_timeout_list.push_back(l_timeout);
                    m_event_list.pop();
                }
                std::sort(m_timeout_list.begin(),m_timeout_list.end(),timeout_sorter);
                auto l_bt = m_timeout_list.begin();
                if(l_bt != m_timeout_list.end())
                {
                    m_best_timer = (*l_bt);

                }
            }
            m_event_mutex.unlock();

            std::unique_lock<std::mutex>l_lock(m_event_mutex);
            if(m_best_timer != nullptr)
            {
                auto l_now = std::chrono::steady_clock::now();
                ///std::cout << "sleep until:" << std::chrono::duration_cast<std::chrono::milliseconds>(m_best_timer->m_deadline - l_now).count() <<  std::endl;
                std::cv_status l_status;
                if(l_now > m_best_timer->m_deadline)
                {
                    // std::cout << "yes"<<std::endl;
                    l_status = std::cv_status::timeout;
                }
                else
                {
                    //std::cout << "wait until" << m_timeout_list.size()<< std::endl;
                    l_status =  m_event_cond.wait_until(l_lock,m_best_timer->m_deadline);
                }
                if(l_status == std::cv_status::timeout)
                {
                    while(!m_event_list.empty())
                    {
                        Timeout*l_timeout = m_event_list.front();
                        m_timeout_list.push_back(l_timeout);
                        m_event_list.pop();
                    }

                    /// execute handlers
                    l_lock.unlock();
                    m_best_timer->action();

                    if(m_best_timer->m_repeat>0)
                    {
                        auto now = std::chrono::steady_clock::now();
                        m_best_timer->m_deadline = now + std::chrono::milliseconds(m_best_timer->m_repeat);
                        std::sort(m_timeout_list.begin(),m_timeout_list.end(),timeout_sorter);
                    }
                    else
                    {
                        auto l_bt = std::find(m_timeout_list.begin(),m_timeout_list.end(),m_best_timer);
                        if(l_bt != m_timeout_list.end())
                        {
                            m_timeout_list.erase(l_bt);
                            std::sort(m_timeout_list.begin(),m_timeout_list.end(),timeout_sorter);
                        }
                        delete m_best_timer;
                    }
                    m_best_timer = nullptr;
                    {
                        auto l_bt = m_timeout_list.begin();
                        if(l_bt != m_timeout_list.end())
                        {
                            m_best_timer = (*l_bt);
                        }
                    }
                }
                else
                {

                    ///std::cout << "not timeout: " << m_event_list.size() << std::endl;
                    while(!m_event_list.empty())
                    {
                        Timeout*l_timeout = m_event_list.front();
                        m_timeout_list.push_back(l_timeout);
                        m_event_list.pop();
                    }
                    std::sort(m_timeout_list.begin(),m_timeout_list.end(),timeout_sorter);
                    auto l_bt = m_timeout_list.begin();
                    if(l_bt != m_timeout_list.end())
                    {
                        m_best_timer = (*l_bt);

                    }
                    l_lock.unlock();


                }
            }
            else
            {
                m_event_cond.wait(l_lock,[&]()->bool
                {
                    /// while this callback is called, lock is locked so it is safe to not use lock here
                    return !m_running || !m_event_list.empty();
                });
                /*while(!m_event_list.empty())
                {
                    Timeout*l_timeout = m_event_list.front();
                    m_timeout_list.push_back(l_timeout);
                    m_event_list.pop();
                }

                std::sort(m_timeout_list.begin(),m_timeout_list.end(),timeout_sorter);
                auto l_bt = m_timeout_list.begin();
                if(l_bt != m_timeout_list.end())
                {
                    m_best_timer = (*l_bt);
                }*/
                l_lock.unlock();
            }
        }/// end of loop
        m_ready = false;
        std::vector<Timeout*> l_evt_list;
        {
            /// get read of all left events since we are closing the loop
            m_event_mutex.lock();
            while(!m_event_list.empty())
            {
                Timeout* l_event = m_event_list.front();
                m_event_list.pop();
                l_evt_list.push_back(l_event);
                //object_check_out(l_event);
            }
            m_event_mutex.unlock();
        }

        {
            auto l_m = l_evt_list.begin();
            while(l_m != l_evt_list.end())
            {
                Timeout*l_timeout = (*l_m);
                delete l_timeout;
                l_evt_list.erase(l_m);
                l_m = l_evt_list.begin();
            }
        }
        {
            auto l_tm = m_timeout_list.begin();
            while(l_tm != m_timeout_list.end())
            {
                Timeout*l_timeout = (*l_tm);
                delete l_timeout;
                m_timeout_list.erase(l_tm);
                l_tm = m_timeout_list.begin();
            }
        }
    }
    virtual~TimerService()
    {
        if(m_thread.joinable())
        {
            m_thread.join();
        }
    }

    void push_event(Timeout*a_evt)
    {
        /// if event is ready/running, we can push event, maybe return a bool will help in some scenarios, will see...
        if(m_ready)
        {
            {
                std::lock_guard<std::mutex> l_lock(m_event_mutex);
                m_event_list.push(a_evt);
            }
            /// notify condition variable that list is updated
            m_event_cond.notify_all();
        }
        else std::cout << "timeout event push declined" << std::endl;
    }
    void create_timeout(EventLoop*a_loop,std::function<void()>a_cb,uint64_t a_duration)
    {
        push_event(new Timeout(a_loop,a_cb,a_duration));
    }
    void create_interval(EventLoop*a_loop,std::function<void()>a_cb,uint64_t a_duration)
    {
        push_event(new Timeout(a_loop,a_cb,a_duration,true));
    }


    void destroy(std::function<void()>a_cb)
    {

        if(m_running)/// if we are running
        {
            create_timeout(nullptr,[&,a_cb]()->void
            {
                m_running = false;
                create_timeout(nullptr,[&]()->void{

                },0);
                if(a_cb != nullptr)
                {
                    a_cb();
                }
            },0);
        }
        else
        {
            if(a_cb != nullptr)
            {
                a_cb();
            }
        }

    }

};

void object_check_out(Object*a_obj,std::function<void()>a_cb)
{
    assert(a_obj != nullptr);
    if(a_obj->check_out(a_cb))
    {
        a_obj->destroy([&,a_obj,a_cb]()
        {
            delete a_obj;
            if(a_cb != nullptr)
            {
                a_cb();
            }
        });
    }
}

void system_init()
{
    g_main_loop = new EventLoop();
    g_main_loop->check_in();
    g_timer_service = new TimerService();
    g_timer_service->check_in();
    g_thread_poll = new Thread_poll();
    g_thread_poll->check_in();
}

void system_run()
{
    g_main_loop->run();
    object_check_out(g_main_loop,[&]()->void
    {
        g_main_loop = nullptr;

    });

    std::cout << "ref_counter:" << ref_counter << std::endl;
    std::cout << "ref_aloc:" << ref_aloc << std::endl;
    std::cout << "ref_dealoc:" << ref_dealoc << std::endl;

}

void system_quit()
{
    std::cout << "cleaning up" << std::endl;
    object_check_out(g_thread_poll,[&]()->void
    {
        std::cout << "thread poll clean up ok" << std::endl;
        g_thread_poll = nullptr;

        object_check_out(g_timer_service,[&]( )->void{
            std::cout << "timer service clean ok"<< std::endl;
            g_timer_service = nullptr;
            g_main_loop->post([&]()->void{
                g_main_loop->stop();
            });
        });
    });

}

std::chrono::steady_clock::time_point g_start;
int main()
{
    system_init();

    g_start = std::chrono::steady_clock::now();
    g_timer_service->create_timeout(nullptr,[&]()->void
    {
        auto l_now = std::chrono::steady_clock::now();
        std::cout << "timeout should be called "<< 10000 << " milliseconds after program start, time paused:" << std::chrono::duration_cast<std::chrono::milliseconds>(l_now - g_start).count() <<  std::endl;
        g_start = l_now;
        system_quit();
    }, 10000);


    g_timer_service->create_timeout(g_main_loop,[&]()->void
    {
        ///  auto l_now = std::chrono::steady_clock::now();
        std::cout << "timeout should run first"  <<  std::endl;
    }, 2000);

    g_timer_service->create_timeout(g_main_loop,[&]()->void
    {
        std::cout << "timeout should run second"  <<  std::endl;
    }, 2000);

    g_timer_service->create_interval(g_main_loop,[&]()->void
    {
        std::cout << "time interval call"  <<  std::endl;
    }, 1000);


    Thread*l_th = g_thread_poll->get();
    if(l_th != nullptr)
    {
        g_main_loop->post([&,l_th]()->void
        {
            std::cout << "call in main thread" << std::endl;
            l_th->post([&,l_th]()->void
            {

                std::cout << "call in other thread" << std::endl;

            });
        });


        g_timer_service->create_timeout(l_th,[&]()->void
        {
            std::cout << "timeout call in other thread " << std::endl;
        }, 3000);

    }





    system_run();






    return 0;
}
