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

EventLoop*g_main_loop = nullptr;


void destroy_object(Object*);
std::mutex g_object_list_mutex;
std::vector<Object*> g_objects_list;
std::vector<Object*> g_subscribe_objects_list;/// for object which needs to receive event when program is about to quit...
std::atomic_int ref_counter;
class Object
{
protected:
    std::atomic_int m_ref_c; /// reference count. >=1  is in g_objects_list
    std::atomic_bool m_sub_l; /// indicator if object in g_subscribe_objects_list
    std::atomic_bool m_des_p; /// indicator if object is in destruction mode
public:

    Object():m_ref_c(0),m_sub_l(false),m_des_p(false)
    {

    }
    virtual~Object()
    {

    }
    virtual void destroy(std::function<void()>a_cb)
    {

        a_cb();
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
    bool check_out()
    {
        m_ref_c--;
        //std::cout << "m_ref_c--: " << m_ref_c<< std::endl;
        /// moving from >=1 to 0 means object will be destroyed
        if(m_ref_c == 0)
        {
            m_des_p = true;/// set m_des_p to true since destroy can call callback in asynchronous way, from other thread.. etc..
            destroy([&]()->void
            {
                //std::cout << "destroying the object " << std::endl;
                std::lock_guard<std::mutex> l_guard(g_object_list_mutex);
                //std::cout << "lock ok" << std::endl;
                if(m_sub_l)
                {
                    m_sub_l = false;
                    auto a = std::find(g_subscribe_objects_list.begin(),g_subscribe_objects_list.end(),this);
                    if(a != g_subscribe_objects_list.end())
                    {
                        g_subscribe_objects_list.erase(a);
                    }
                }
                auto b = std::find(g_objects_list.begin(),g_objects_list.end(),this);
                if(b != g_objects_list.end())
                {
                    //std::cout << "deleting the object " << std::endl;
                    delete (*b);
                    g_objects_list.erase(b);
                }
                else {
                    std::cout << "WARNING: did not find my place in object list" << std::endl;
                }
            });
        }
        /// < 0 means object did not ever been on g_objects_list, manual deletion is needed in this case
        else if(m_ref_c <0&&!m_des_p)
        {
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
class Event : public Object
{
public:
    Event():Object() {}
    virtual~Event() {}
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
    virtual~EventLoop() {}

    /// in case where we need to integrate with other frameworks this can help to become flexible
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
            destroy_object((*l_e));
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
            std::cout << "lock:" << m_running << std::endl;
            m_event_cond.wait(l_lock

                              ,[&]()->bool
            {
                /// while this callback is called, lock is locked so it is safe to not use lock here
                return !m_running || !m_event_list.empty();
            });
            std::cout << "unlock" << std::endl;
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
                destroy_object((*l_e));
                l_ready_events.erase(l_e);
                l_e = l_ready_events.begin();
            }
            // std::cout << "loop end" << std::endl;
        }/// end of loop

        std::vector<Event*> l_evt_list;
        {
            /// get read of all left events since we are closing the loop
            m_event_mutex.lock();
            while(!m_event_list.empty())
            {
                Event* l_event = m_event_list.front();
                m_event_list.pop();
                l_evt_list.push_back(l_event);
                //destroy_object(l_event);
            }
            m_event_mutex.unlock();
        }

        {
            auto l_m = l_evt_list.begin();
            while(l_m != l_evt_list.end())
            {
                Event* l_event = (*l_m);
                destroy_object(l_event);
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
    Thread():EventLoop(),m_thread([&]
    {
        m_event_mutex.lock();
        m_promise.set_value();
        m_event_mutex.unlock();
        run();

        //  std::cout << "run method return" << std::endl;
    })
    {
        m_event_mutex.lock();
        std::future<void> l_future = m_promise.get_future();
        m_event_mutex.unlock();
        l_future.wait();
        ref_counter++;
        std::cout << "thread in : " << ref_counter << std::endl;
    }
    virtual~Thread()
    {

        ref_counter--;
        m_event_cond.notify_all();
        if(m_thread.joinable())
        {
            std::cout << "join" << std::endl;
            m_thread.join();/// when joining the thread very rare friezes the process, not sure why
            std::cout << "join done" << std::endl;
        }
        std::cout << "thread out: " << ref_counter << std::endl;

    }
    void destroy(std::function<void()>a_cb)
    {
        if(is_running())
        {
            EventLoop::destroy([&,a_cb]()->void
            {
                g_main_loop->post([&,a_cb]()->void
                {
                    m_running = false;
                    m_ready = false;
                    m_event_cond.notify_all();
                    if(a_cb != nullptr)
                    {
                        a_cb();
                    }
                });
            });
        }
        else
        {
            std::cout << "not running, normal exit" << std::endl;
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
        std::cout <<"thread poll out:" << ref_counter << std::endl;
    }
    Thread*get()
    {
        if(m_des_p)
        {
            return nullptr;
        }
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
    void remove(Thread*a_th)
    {
        std::lock_guard<std::mutex>l_guard(m_thread_list_mutex);
        auto l_th = m_thread_list.find(a_th);
        if(l_th != m_thread_list.end())
        {
            if(l_th->second)
            {
                m_free_threads--;
            }
            l_th->first->check_out();
            m_thread_list.erase(l_th);
        }
    }
    void destroy(std::function<void()>a_cb)
    {
        std::map<Thread*,bool> l_thread_list;
        {
            std::lock_guard<std::mutex>l_guard(m_thread_list_mutex);
            auto l_th = m_thread_list.begin();
            while(l_th != m_thread_list.end())
            {
                l_thread_list.insert(std::pair<Thread*,bool>(l_th->first,l_th->second));
                m_thread_list.erase(l_th);
                l_th = m_thread_list.begin();
            }
        }
        {
            auto l_th = l_thread_list.begin();
            while(l_th != l_thread_list.end())
            {
                l_th->first->check_out();
                l_thread_list.erase(l_th);
                l_th = l_thread_list.begin();
            }
            if(a_cb != nullptr)
            {
                a_cb();
            }
        }
    }
};

class TimerService : public Object
{
protected:
    std::queue<Event*> m_event_list;
    std::mutex m_event_mutex;
    std::condition_variable m_event_cond;
public:
    TimerService():Object() {}
    virtual~TimerService() {}
    void destroy(std::function<void()>a_cb)
    {

    }

};

void destroy_object(Object*a_obj)
{
    assert(a_obj != nullptr);
    if(a_obj->check_out())
    {
        a_obj->destroy([&,a_obj]()
        {
            delete a_obj;
        });
    }
}

int main()
{

    g_main_loop = new EventLoop();
    Thread_poll*l_thread_poll = new Thread_poll();
    l_thread_poll->check_in();
    Thread*l_thread = l_thread_poll->get();
    l_thread_poll->get();
    l_thread_poll->get();
    l_thread_poll->get();
    l_thread_poll->get();
    l_thread->post([&,l_thread,l_thread_poll]()->void
    {
        std::cout << "thread hello world" << std::endl;
        destroy_object(l_thread_poll);
    });

    g_main_loop->post([&]()->void
    {
    });
    g_main_loop->run();


    return 0;
}
