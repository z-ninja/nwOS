#include <base/bree.h>
#include <thread>
namespace bree
{

enum timer_action
{
    timer_action_timeout          = 1 << 0,
    timer_action_interval         = 1 << 1,
    timer_action_canceled         = 1 << 2,

};

enum manager_action
{
    manager_action_timer_cancel             = 1 << 3,
    manager_action_timer                    = 1 << 4,
    manager_action_context_main_register    = 1 << 5,
    manager_action_context_register         = 1 << 6,
    manager_action_thread_register          = 1 << 7,
    manager_action_thread_unregister        = 1 << 8,
    manager_action_context_done             = 1 << 9,
    manager_action_bree_quit                = 1 << 10,
    manager_action_bree_closed              = 1 << 11
};
struct manager_action_thread : public bree_event
{
    bree_context_thread_ptr m_thread;
    manager_action_thread(bree_context_thread_ptr a_thread,manager_action a_action):bree_event(a_action),m_thread(a_thread) {}
    virtual~manager_action_thread() {}
};

struct timer_event : public bree_event
{
    std::function<void()> m_cb;
    std::chrono::steady_clock::time_point m_deadline;
    int64_t m_duration;
    timer_event(std::function<void()>a_cb,int64_t a_duration,timer_action a_timer_action,bree_context_ptr a_context):
        bree_event(manager_action_timer|a_timer_action,a_context),m_cb(a_cb),m_deadline(),m_duration(a_duration)
    {
        auto now = std::chrono::steady_clock::now();
        m_deadline = now+std::chrono::milliseconds(a_duration);
    }
    virtual~timer_event() {}
    void dispatch()
    {
        if(m_dest != nullptr)
        {
            m_dest->post(m_cb,true);
        }
    }
};

struct timer_event_cancel : public bree_event
{
    timer::timer_t m_timer_h;
    timer_event_cancel(timer::timer_t a_timer_h):bree_event(manager_action_timer_cancel),m_timer_h(a_timer_h) {}
    virtual~timer_event_cancel() {}
};
struct manager_context_action_register : public bree_event
{
    bree_context_ptr m_context;
    bree::object_unregister_callback m_clean_up_callback;
    manager_context_action_register(bree_context_ptr a_context,bree::object_unregister_callback a_clean_up_callback,manager_action a_action):
        bree_event(a_action),m_context(a_context),m_clean_up_callback(a_clean_up_callback)
    {}
    virtual~manager_context_action_register() {}
};
struct manager_context_action_event : public bree_event
{
    bree_context_ptr m_context;
    manager_context_action_event(bree_context_ptr a_context,manager_action a_action):bree_event(a_action),m_context(a_context)
    {}
    virtual~manager_context_action_event() {}
};
typedef timer_event*timer_event_ptr;
class bree_context_manager
{
private:

    void work()
    {
        struct
        {
            bool operator()(timer_event_ptr a, timer_event_ptr b) const
            {
                return a->m_deadline < b->m_deadline;
            }
        } timers_sorter;
        std::unique_lock<std::mutex> l_lock(m_event_mutex);
        std::vector<bree_event_ptr> l_events_list;
        std::map<bree_context_ptr,bree::object_unregister_callback> l_context_cache;

        m_running = true;
        bool l_bool_quit_event = false;
        bool main_context_registerd = false;
        while(m_running)
        {
            if(m_best_timer != nullptr)
            {

                auto l_now = std::chrono::steady_clock::now();
                std::cv_status l_status;
                if(l_now >= m_best_timer->m_deadline)
                {
                    l_status = std::cv_status::timeout;
                }
                else
                {
                    l_status =  m_event_cond.wait_until(l_lock,m_best_timer->m_deadline);
                }
                if(l_status == std::cv_status::timeout)
                {
                    l_lock.unlock();
                    if(l_bool_quit_event){
                      m_best_timer->m_flags |= timer_action_canceled;
                    }
                    if(m_best_timer->m_flags & timer_action_canceled)
                    {
                        /// canceled action
                        auto l_tm = std::find_if(m_timers_list.begin(),m_timers_list.end(),[&](const timer_event*this_timer)->bool
                        {
                            return m_best_timer == this_timer;
                        });
                        if(l_tm != m_timers_list.end())
                        {
                            m_timers_list.erase(l_tm);
                            delete m_best_timer;
                        }
                        else {
                         /// should not happen
                                std::cout << "WARNING:could not find timer in list to cancel" << std::endl;
                        }
                        m_best_timer = nullptr;
                    }
                    else
                    {
                        m_best_timer->dispatch();
                    }
                    if(m_best_timer != nullptr)
                    {

                        if(m_best_timer->m_flags & timer_action_interval)
                        {
                            m_best_timer->m_deadline +=std::chrono::milliseconds(m_best_timer->m_duration);
                            m_best_timer = nullptr;
                            std::sort(m_timers_list.begin(),m_timers_list.end(),timers_sorter);
                        }
                        else  if(m_best_timer->m_flags & timer_action_timeout)
                        {
                            auto l_tm = std::find_if(m_timers_list.begin(),m_timers_list.end(),[&](const timer_event*this_timer)->bool
                            {
                                return m_best_timer == this_timer;
                            });
                            if(l_tm != m_timers_list.end())
                            {
                                m_timers_list.erase(l_tm);
                                delete m_best_timer;
                            }
                            else{
                            /// should not happen
                            std::cout << "WARNING:could not find timer in list" << std::endl;
                            }

                            m_best_timer = nullptr;
                        }
                        else
                        {

                            std::cout << "invalid event type for timer" << std::endl;
                        }
                    }

                    if(m_best_timer == nullptr)
                    {
                        auto l_best_timer = m_timers_list.begin();
                        if(l_best_timer != m_timers_list.end())
                        {
                            m_best_timer = (*l_best_timer);
                        }
                    }
                }
                else
                {
                    size_t new_timers_count = 0;
                    while(!m_event_list.empty())
                    {
                        bree_event_ptr w_evt = m_event_list.front();
                        m_event_list.pop();
                        if(w_evt->m_flags & manager_action_timer)
                        {
                            timer_event*w_timer = dynamic_cast<timer_event*>(w_evt);
                            m_timers_list.push_back(w_timer);
                            new_timers_count++;
                        }
                        else
                        {
                            l_events_list.push_back(w_evt);
                        }
                    }
                    l_lock.unlock();

                    if(new_timers_count>0)
                    {
                        std::sort(m_timers_list.begin(),m_timers_list.end(),timers_sorter);
                        auto l_best_timer = m_timers_list.begin();
                        if(l_best_timer != m_timers_list.end())
                        {
                            m_best_timer = (*l_best_timer);
                        }
                    }
                }
            }
            else
            {
                m_event_cond.wait(l_lock,[&]()->bool
                {
                    /// while this callback is called, lock is locked so it is safe to not use lock here
                    return !m_running || !m_event_list.empty();
                });
                /// here lock is still locked by so far
                size_t new_timers_count = 0;
                while(!m_event_list.empty())
                {
                    bree_event_ptr w_evt = m_event_list.front();
                    m_event_list.pop();
                    if(w_evt->m_flags & manager_action_timer)
                    {
                        timer_event*w_timer = dynamic_cast<timer_event*>(w_evt);
                        m_timers_list.push_back(w_timer);
                        new_timers_count++;
                    }
                    else
                    {
                        l_events_list.push_back(w_evt);
                    }
                }
                if(new_timers_count>0)
                {
                    std::sort(m_timers_list.begin(),m_timers_list.end(),timers_sorter);
                }
                auto l_best_timer = m_timers_list.begin();
                if(l_best_timer != m_timers_list.end())
                {
                    m_best_timer = (*l_best_timer);
                }
                l_lock.unlock();
            }
            auto l_ev = l_events_list.begin();
            while(l_ev != l_events_list.end())
            {
                bree_event_ptr w_evt = (*l_ev);

                if(w_evt->m_flags & manager_action_timer_cancel)
                {
                    timer_event_cancel*l_evt = dynamic_cast<timer_event_cancel*>(w_evt);
                    timer_event*l_canceled_timer = reinterpret_cast<timer_event*>(l_evt->m_timer_h);
                    l_canceled_timer->m_flags |= timer_action_canceled;
                    auto now = std::chrono::steady_clock::now() - std::chrono::milliseconds(1);
                    l_canceled_timer->m_deadline = now;
                    m_best_timer = nullptr;
                    std::sort(m_timers_list.begin(),m_timers_list.end(),timers_sorter);
                    auto l_best_timer = m_timers_list.begin();
                    if(l_best_timer != m_timers_list.end())
                    {
                        m_best_timer = (*l_best_timer);
                    }

                }
                else if(w_evt->m_flags & manager_action_context_register)
                {
                    manager_context_action_register*l_evt = dynamic_cast<manager_context_action_register*>(w_evt);
                    if(main_context_registerd&&m_context != nullptr)
                    {
                        l_evt->m_context->to_object();
                        context::register_object(m_context,l_evt->m_context->to_object(),l_evt->m_clean_up_callback);
                    }
                    else
                    {
                        if(l_context_cache.find(l_evt->m_context) == l_context_cache.end())
                        {
                            l_context_cache.insert(std::make_pair(l_evt->m_context,l_evt->m_clean_up_callback));
                        }
                    }
                }
                else if(w_evt->m_flags & manager_action_context_main_register)
                {
                    manager_context_action_event*l_evt = dynamic_cast<manager_context_action_event*>(w_evt);
                    m_context = l_evt->m_context;
                    auto l_mc = l_context_cache.find(m_context);
                    if(l_mc != l_context_cache.end())
                    {
                        m_main_context_clean_up_callback = l_mc->second;
                        l_context_cache.erase(l_mc);
                        main_context_registerd = true;
                    }
                    l_mc = l_context_cache.begin();
                    while(l_mc != l_context_cache.end())
                    {
                        register_context(l_mc->first,l_mc->second);
                        l_context_cache.erase(l_mc);
                        l_mc = l_context_cache.begin();
                    }
                }
                else if(w_evt->m_flags & manager_action_thread_register)
                {
                    manager_action_thread*l_evt = dynamic_cast<manager_action_thread*>(w_evt);
                    if(l_evt->m_thread != nullptr)
                    {
                        if(std::find(m_threads_list.begin(),m_threads_list.end(),l_evt->m_thread)== m_threads_list.end())
                        {
                            m_threads_list.push_back(l_evt->m_thread);
                        }
                    }
                }
                else if(w_evt->m_flags & manager_action_thread_unregister)
                {
                    manager_action_thread*l_evt = dynamic_cast<manager_action_thread*>(w_evt);
                    if(l_evt->m_thread != nullptr)
                    {
                        auto l_th = std::find(m_threads_list.begin(),m_threads_list.end(),l_evt->m_thread);

                        if(l_th != m_threads_list.end())
                        {
                            if((*l_th)->m_thread.joinable()){
                            (*l_th)->m_thread.join();
                            }
                            m_threads_list.erase(l_th);
                        }
                    }
                    if(l_bool_quit_event)
                    {
                        if(m_threads_list.size() == 0)
                        {
                            m_running = false;
                        }
                    }
                }
                else if(w_evt->m_flags & manager_action_context_done)
                {
                    if(l_bool_quit_event)
                    {
                        if(m_threads_list.size() == 0)
                        {
                            m_running = false;
                        }
                    }
                    m_main_context_clean_up_callback(m_context);
                }
                else if(w_evt->m_flags & manager_action_bree_quit)
                {
                    auto l_tm = m_timers_list.begin();

                    while(l_tm != m_timers_list.end())
                    {
                        delete (*l_tm);
                        m_timers_list.erase(l_tm);
                        l_tm = m_timers_list.begin();
                    }
                    l_bool_quit_event = true;
                    m_best_timer = nullptr;
                    m_context->stop();

                }
                l_events_list.erase(l_ev);
                l_ev = l_events_list.begin();
                delete w_evt;
            }
            l_lock.lock();
        }
        m_flags |= manager_action_bree_closed;
        l_lock.unlock();
        while(!m_event_list.empty())
        {
            bree_event_ptr w_evt = m_event_list.front();
            m_event_list.pop();
            l_events_list.push_back(w_evt);
        }
        auto l_ev = l_events_list.begin();
        while(l_ev != l_events_list.end())
        {
            bree_event_ptr w_evt = (*l_ev);
            l_events_list.erase(l_ev);
            l_ev = l_events_list.begin();
            delete w_evt;
        }
    }
public:
    int&m_argc;
    char **&m_argv;
    bree_context_ptr m_context;
    bree::object_unregister_callback m_main_context_clean_up_callback;
    bree_context_manager(int&a_argc,char**&a_argv,bree_context_ptr a_context):m_argc(a_argc),
        m_argv(a_argv),m_context(a_context),m_main_context_clean_up_callback(nullptr),
/** private initialization */
        m_running(false),m_event_mutex(),m_event_cond(),m_timers_list(),m_threads_list(),
        m_best_timer(nullptr),m_thread([&]()->void
    {
        work();
    })
    {
        if(m_context == nullptr)
        {
            m_context = bree::context::context_new();
        }
        m_context->m_flags |= bree::context_flags_main;
        this_thread::context = m_context;
        instance = this;
        manager_context_action_event* l_evt = new manager_context_action_event(m_context,manager_action_context_main_register);
        push_event(l_evt);
    }
    ~bree_context_manager()
    {

    }
    static bool push_event(bree_event_ptr a_event)
    {
        if(instance != nullptr)
        {
            {
                std::unique_lock<std::mutex> l_lock(instance->m_event_mutex);
                if(m_flags & manager_action_bree_closed)
                {
                    l_lock.unlock();
                    delete a_event;
                    return false;
                }
                m_event_list.push(a_event);
            }
            instance->m_event_cond.notify_one();
        }
        else
        {
            if(m_flags & manager_action_bree_closed)
            {
                delete a_event;
                return false;
            }
            m_event_list.push(a_event);
        }
        return true;
    }
    static bool register_context(bree_context_ptr a_context,bree::object_unregister_callback a_cb)
    {
        /// event
        manager_context_action_register* l_evt = new manager_context_action_register(a_context,a_cb,manager_action_context_register);
        return push_event(l_evt);
    }

    static void bree_init(int&a_argc,char**&a_argv,bree_context_ptr a_context)
    {
        if(a_context != nullptr)
        {
            a_context->m_flags |= bree::context_flags_main;
        }
        static bree_context_manager manager(a_argc,a_argv,a_context);

    }

    static void bree_run()
    {
        if(instance != nullptr)
        {
            std::unique_lock<std::mutex> l_lock(instance->m_event_mutex);
            if(m_flags & manager_action_bree_closed)
            {
                l_lock.unlock();
                return;
            }
            l_lock.unlock();
            instance->m_context->run();
            try{
            if(instance->m_thread.joinable())
            {
                instance->m_thread.join();
            }
            }catch(std::exception&ex){
            std::cout << "timer thread join exception: " << ex.what() << std::endl;
            /// should never happen
            }
        }
        this_thread::context = nullptr;
    }

    static void bree_exit(int a_exit)
    {
        s_exit_code = a_exit;
        push_event(new bree_event(manager_action_bree_quit));
    }


    static void interval_callback(std::function<bool()>a_cb,timer::timer_t a_handle)
    {
        if(a_cb())
        {
            bree_context_manager::cancel(a_handle);
        }
    }
    static timer::timer_t timeout(bree_context_ptr a_context,std::function<void()>a_cb,int64_t a_milliseconds)
    {
        if(a_context == nullptr)
        {
            if(instance == nullptr|| instance->m_context == nullptr)
            {
                return nullptr;
            }
            a_context = instance->m_context;
        }
        timer_event_ptr l_evt = new timer_event(a_cb,a_milliseconds,timer_action_timeout,a_context);
        timer::timer_t l_timer_h = reinterpret_cast<timer::timer_t>(l_evt);
        push_event(l_evt);
        return l_timer_h;
    }
    static timer::timer_t interval(bree_context_ptr a_context,std::function<bool()>a_cb,int64_t a_milliseconds)
    {
        if(a_context == nullptr)
        {
            if(instance == nullptr|| instance->m_context == nullptr)
            {
                return nullptr;
            }
            a_context = instance->m_context;
        }
        timer_event_ptr l_evt = new timer_event(nullptr,a_milliseconds,timer_action_interval,a_context);
        timer::timer_t l_timer_h = reinterpret_cast<timer::timer_t>(l_evt);
        std::function<void()> l_cb = std::bind(&bree_context_manager::interval_callback,a_cb,l_timer_h);
        l_evt->m_cb = l_cb;
        push_event(l_evt);
        return l_timer_h;
    }
    static void cancel(bree::timer::timer_t a_timer)
    {
        timer_event_cancel*l_evt = new timer_event_cancel(a_timer);
        push_event(l_evt);
    }
    static bree_context_manager*instance;
    static std::queue<bree_event_ptr> m_event_list;
    static int m_flags;
    static int s_exit_code;

private:
    bool m_running;
    std::mutex m_event_mutex;
    std::condition_variable m_event_cond;
    std::vector<timer_event_ptr> m_timers_list;
    std::vector<bree_context_thread_ptr> m_threads_list;
    timer_event_ptr m_best_timer;
    std::thread m_thread;
};

bree_context_manager* bree_context_manager::instance = nullptr;
std::queue<bree_event_ptr> bree_context_manager::m_event_list;
int bree_context_manager::m_flags = 0;
int bree_context_manager::s_exit_code = 0;


BREE_PUBLIC void bree_init(int&a_argc,char**&a_argv,bree_context_ptr a_context)
{
    bree_context_manager::bree_init(a_argc,a_argv,a_context);
}
BREE_PUBLIC void bree_run()
{
    bree_context_manager::bree_run();
}
BREE_PUBLIC void bree_exit(int a_exit)
{
    bree_context_manager::bree_exit(a_exit);
}
BREE_PUBLIC int bree_exit_code()
{
    return bree_context_manager::s_exit_code;
}

namespace timer
{
BREE_PUBLIC timer_t timeout(bree_context_ptr a_context,std::function<void()>a_cb,int64_t a_milliseconds)
{
    return bree_context_manager::timeout(a_context,a_cb,a_milliseconds);
}
BREE_PUBLIC timer_t interval(bree_context_ptr a_context,std::function<bool()>a_cb,int64_t a_milliseconds)
{
    return bree_context_manager::interval(a_context,a_cb,a_milliseconds);
}
BREE_PUBLIC void cancel(timer_t a_timer)
{
    bree_context_manager::cancel(a_timer);
}
}/// namespace timer

BREE_PUBLIC int&get_argc()
{
    return bree_context_manager::instance->m_argc;
}
BREE_PUBLIC char**&get_argv()
{
    return bree_context_manager::instance->m_argv;
}
namespace context
{


namespace global
{
BREE_PUBLIC void register_context(bree_context_ptr a_context,bree::object_unregister_callback a_cb)
{
    bree_context_manager::register_context(a_context,a_cb);
}
BREE_PUBLIC void main_context_done(bree_context_ptr a_context)
{
    if(bree_context_manager::instance != nullptr)
    {
        bree_context_manager::instance->push_event(new manager_context_action_event(a_context,bree::manager_action_context_done));
    }
}
BREE_PUBLIC void register_thread(bree_context_thread_ptr a_thread)
{
    if(bree_context_manager::instance != nullptr)
    {
        bree_context_manager::instance->push_event(new manager_action_thread(a_thread,manager_action_thread_register));
    }
}
BREE_PUBLIC void unregister_thread(bree_context_thread_ptr a_thread)
{
    if(bree_context_manager::instance != nullptr)
    {
        bree_context_manager::instance->push_event(new manager_action_thread(a_thread,manager_action_thread_unregister));
    }
}
}/// namespace global
}/// namespace context
namespace main_thread
{
BREE_PUBLIC bree::timer::timer_t timeout(std::function<void()>a_cb,int64_t a_milliseconds)
{
    return bree::bree_context_manager::timeout(nullptr,a_cb,a_milliseconds);
}
BREE_PUBLIC bree::timer::timer_t interval(std::function<bool()>a_cb,int64_t a_milliseconds)
{
    return bree::bree_context_manager::interval(nullptr,a_cb,a_milliseconds);
}

}
namespace this_thread
{
BREE_PUBLIC bree::timer::timer_t timeout(std::function<void()>a_cb,int64_t a_milliseconds)
{
    return bree::bree_context_manager::timeout(context,a_cb,a_milliseconds);
}
BREE_PUBLIC bree::timer::timer_t interval(std::function<bool()>a_cb,int64_t a_milliseconds)
{
    return bree::bree_context_manager::interval(context,a_cb,a_milliseconds);
}
}/// namespace this_thread
}/// namespace bree
