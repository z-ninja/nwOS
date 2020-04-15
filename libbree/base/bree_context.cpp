#include <base/bree_context.h>

struct bree_context_post_event : public bree_event
{
    std::function<void()>m_cb;
    bree_context_post_event(std::function<void()>a_cb):bree_event(bree::context_event_flag_post),m_cb(a_cb) {}
    virtual~bree_context_post_event() {}
};
struct bree_context_register_event : public bree_event
{
    bree_object_ptr m_object;
    bree::object_unregister_callback m_clean_up_callback;
    bree_context_register_event(bree_object_ptr a_object,bree::object_unregister_callback a_clean_up_callback):bree_event(bree::context_event_flag_register_object|bree::context_event_flag_system_priority),
        m_object(a_object),m_clean_up_callback(a_clean_up_callback) {}
    virtual~bree_context_register_event() {}
};
struct bree_context_unregister_event : public bree_event
{
    bree_object_ptr m_object;
    bree_context_unregister_event(bree_object_ptr a_object):bree_event(bree::context_event_flag_unregister_object|bree::context_event_flag_system_priority),
        m_object(a_object) {}
    virtual~bree_context_unregister_event() {}
};

struct bree_context_quit_event : public bree_event
{
    bree_context_ptr m_context;
    bree_context_quit_event(bree_context_ptr a_context):bree_event(bree::context_event_flag_quit| bree::context_event_flag_system_priority),
        m_context(a_context) {}
    virtual~bree_context_quit_event() {}
};



bree_context::bree_context():bree_object(),m_running(false),m_mutex(),
    m_con_var(),m_event_list(),m_high_priority_list(),m_system_priority_list(),m_objects_list(),m_parent_context(nullptr)
{
}
bree_context::~bree_context()
{
}
bool bree_context::closed()
{
    return (m_flags & bree::context_flags_state_closed);
}
void bree_context::stop()
{

    push_event(new bree_context_quit_event(nullptr));

}
size_t bree_context::run_once()
{
    std::unique_lock<std::mutex> l_lock(m_mutex);
    if(!m_running)
    {
        if(!closed())
        {
            m_running = true;
        }
        else
        {
            l_lock.unlock();
            return 0;
        }
    }
    size_t execute_handler_count = 0;
    bool l_bool_quit_event = (m_flags & bree::context_event_flag_exit_requested);
    if((!l_bool_quit_event&&(m_high_priority_list.size()||m_event_list.size()))||m_system_priority_list.size())
    {
        std::vector<bree_event_ptr> l_event_list;

        while(!m_system_priority_list.empty())
        {
            bree_event_ptr w_evt = m_system_priority_list.front();
            m_system_priority_list.pop();
            l_event_list.push_back(w_evt);
        }
        execute_handler_count  = l_event_list.size();
        while((execute_handler_count==0)&&!m_high_priority_list.empty()&&execute_handler_count<100)
        {
            bree_event_ptr w_evt = m_high_priority_list.front();
            m_high_priority_list.pop();
            l_event_list.push_back(w_evt);
            execute_handler_count++;
        }
        execute_handler_count  = l_event_list.size();

        while((execute_handler_count==0)&&!m_event_list.empty())
        {
            bree_event_ptr w_evt = m_event_list.front();
            m_event_list.pop();
            l_event_list.push_back(w_evt);
        }
        l_lock.unlock();
        execute_handler_count = 0;

        auto l_iter = l_event_list.begin();
        while(l_iter != l_event_list.end())
        {
            bree_event_ptr w_evt = (*l_iter);
            if(w_evt->m_flags & bree::context_event_flag_post)
            {
                bree_context_post_event*l_ex_evt = dynamic_cast<bree_context_post_event*>(w_evt);
                if(l_ex_evt->m_cb != nullptr)
                {
                    l_ex_evt->m_cb();
                }
                delete l_ex_evt;
            }
            else if(w_evt->m_flags & bree::context_event_flag_register_object)
            {
                bree_context_register_event*l_levt = dynamic_cast<bree_context_register_event*>(w_evt);
                if(l_levt->m_object != nullptr&&m_objects_list.find(l_levt->m_object)== m_objects_list.end())
                {
                    m_objects_list.insert(std::make_pair(l_levt->m_object,l_levt->m_clean_up_callback));
                }
            }
            else if(w_evt->m_flags & bree::context_event_flag_unregister_object)
            {
                bree_context_unregister_event*l_levt = dynamic_cast<bree_context_unregister_event*>(w_evt);
                if(l_levt->m_object != nullptr)
                {
                    auto l_pair = m_objects_list.find(l_levt->m_object);
                    if(l_pair != m_objects_list.end())
                    {
                        l_pair->second(l_pair->first);
                        m_objects_list.erase(l_pair);
                    }
                }
                if(l_bool_quit_event)
                {
                    if(m_objects_list.size() == 0)
                    {
                        m_running = false;
                    }
                }

            }
            else if(w_evt->m_flags & bree::context_event_flag_quit)
            {
                if(!l_bool_quit_event)
                {
                    bree_context_quit_event*l_evt = dynamic_cast<bree_context_quit_event*>(w_evt);
                    if(l_evt->m_context != nullptr)
                    {
                        m_parent_context = l_evt->m_context;
                    }
                    l_bool_quit_event = true;
                    bree_context_ptr l_me = shared_from_this();
                    for(auto f_e = m_objects_list.begin(); f_e != m_objects_list.end(); f_e++)
                    {
                        f_e->first->exit(l_me);
                    }
                    if(m_objects_list.size() == 0)
                    {
                        m_running = false;
                    }
                }
            }
            else
            {
                if(!on_event(w_evt))
                {
                    std::cout << "invalid event" << std::endl;
                }
            }
            l_event_list.erase(l_iter);
            l_iter = l_event_list.begin();
        }
        l_lock.lock();
        if(l_bool_quit_event)
        {
            m_flags |= bree::context_event_flag_exit_requested;
        }
    }
    if(!m_running)
    {
        m_flags |= bree::context_flags_state_closed;
    }
    bool is_main = (m_flags & bree::context_flags_main);
    l_lock.unlock();


    if(!m_running)
    {
        if(is_main)
        {
            bree::context::global::main_context_done(shared_from_this());
        }
        else if(m_parent_context != nullptr)
        {
            bree::context::unregister_object(m_parent_context,to_object());
            m_parent_context = nullptr;
        }

    }
    /// l_parent_context


    return 0;
}
bool bree_context::on_event(bree_event_ptr)
{

    return false;
}
void bree_context::exit(bree_context_ptr a_context)
{

    push_event(new bree_context_quit_event(a_context));
}
void bree_context::run()
{

    std::unique_lock<std::mutex> l_lock(m_mutex);
    m_running = true;
    bool l_bool_quit_event = false;
    std::vector<bree_event_ptr> l_event_list;
    while(m_running||m_system_priority_list.size())
    {
        m_con_var.wait(l_lock,[&]()->bool
        {
            return !m_event_list.empty() || !m_high_priority_list.empty() || !m_running || !m_system_priority_list.empty();
        });

        while(!m_system_priority_list.empty())
        {
            bree_event_ptr w_evt = m_system_priority_list.front();
            m_system_priority_list.pop();
            l_event_list.push_back(w_evt);
        }
        size_t l_high_priority_count  = l_event_list.size();
        while((l_high_priority_count == 0)&&!m_high_priority_list.empty())
        {
            bree_event_ptr w_evt = m_high_priority_list.front();
            m_high_priority_list.pop();
            l_event_list.push_back(w_evt);
        }
         l_high_priority_count  = l_event_list.size();

        while((l_high_priority_count==0)&&!m_event_list.empty())
        {
            bree_event_ptr w_evt = m_event_list.front();
            m_event_list.pop();
            l_event_list.push_back(w_evt);
        }
        l_lock.unlock();
        auto l_iter = l_event_list.begin();
        while(l_iter != l_event_list.end())
        {
            bree_event_ptr w_evt = (*l_iter);
            if(w_evt->m_flags & bree::context_event_flag_post)
            {
                if(!l_bool_quit_event){
                bree_context_post_event*l_ex_evt = dynamic_cast<bree_context_post_event*>(w_evt);
                if(l_ex_evt->m_cb != nullptr)
                {
                    l_ex_evt->m_cb();
                }
                }
            }
            else if(w_evt->m_flags & bree::context_event_flag_register_object)
            {
                bree_context_register_event*l_levt = dynamic_cast<bree_context_register_event*>(w_evt);
                if(l_levt->m_object != nullptr&&m_objects_list.find(l_levt->m_object)== m_objects_list.end())
                {
                    m_objects_list.insert(std::make_pair(l_levt->m_object,l_levt->m_clean_up_callback));
                }
            }
            else if(w_evt->m_flags & bree::context_event_flag_unregister_object)
            {
                bree_context_unregister_event*l_levt = dynamic_cast<bree_context_unregister_event*>(w_evt);
                if(l_levt->m_object != nullptr)
                {
                    auto l_pair = m_objects_list.find(l_levt->m_object);
                    if(l_pair != m_objects_list.end())
                    {
                        l_pair->second(l_pair->first);
                        m_objects_list.erase(l_pair);
                    }
                }
                if(l_bool_quit_event)
                {
                    if(m_objects_list.size() == 0)
                    {
                        m_running = false;
                    }
                }

            }
            else if(w_evt->m_flags & bree::context_event_flag_quit)
            {
                std::cout << "quit event in context: " << l_event_list.size() << std::endl;
                if(!l_bool_quit_event)
                {
                    bree_context_quit_event*l_evt = dynamic_cast<bree_context_quit_event*>(w_evt);
                    if(l_evt->m_context != nullptr)
                    {
                        m_parent_context = l_evt->m_context;
                    }
                    l_bool_quit_event = true;
                    bree_context_ptr l_me = shared_from_this();
                    for(auto f_e = m_objects_list.begin(); f_e != m_objects_list.end(); f_e++)
                    {
                        f_e->first->exit(l_me);
                    }
                    if(m_objects_list.size() == 0)
                    {
                        m_running = false;
                    }
                }
            }
            else
            {
                if(!on_event(w_evt))
                {
                    std::cout << "invalid event" << std::endl;
                }
            }
            l_event_list.erase(l_iter);
            l_iter = l_event_list.begin();
            delete w_evt;
        }
        l_lock.lock();
        if(l_bool_quit_event)
        {
            m_flags |= bree::context_event_flag_exit_requested;
        }
    }
    m_flags |= bree::context_flags_state_closed;
    /// set flag to closed
    bool is_main = (m_flags & bree::context_flags_main);
    l_lock.unlock();


    while(!m_system_priority_list.empty())
    {
        bree_event_ptr w_evt = m_system_priority_list.front();
        m_system_priority_list.pop();
        delete w_evt;
    }
    while(!m_high_priority_list.empty())
    {
        bree_event_ptr w_evt = m_high_priority_list.front();
        m_high_priority_list.pop();
        delete w_evt;
    }
    while(!m_event_list.empty())
    {
        bree_event_ptr w_evt = m_event_list.front();
        m_event_list.pop();
        delete w_evt;
    }

    auto l_iter = l_event_list.begin();
    while(l_iter != l_event_list.end())
    {
        bree_event_ptr w_evt = (*l_iter);
        l_event_list.erase(l_iter);
        l_iter = l_event_list.begin();
        delete w_evt;
    }
    if(is_main)
    {
        bree::context::global::main_context_done(shared_from_this());
    }
    else if(m_parent_context != nullptr)
    {
        bree::context::unregister_object(m_parent_context,to_object());
        m_parent_context =nullptr;
    }

}


bool bree_context::post(std::function<void()>a_cb,bool a_high_priority)
{
    if(a_cb != nullptr)
    {
        bree_context_post_event*l_evt = new bree_context_post_event(a_cb);
        if(a_high_priority)
        {
            l_evt->m_flags |= bree::context_event_flag_high_priority;
        }
        return push_event(l_evt);
    }
    return false;
}
bool bree_context::push_event(bree_event_ptr a_evt)
{

    {
        std::unique_lock<std::mutex> l_lock(m_mutex);

        if(m_flags & bree::context_flags_state_closed)
        {
            l_lock.unlock();
            delete a_evt;
            return false;
        }
        if(a_evt->m_flags & bree::context_event_flag_system_priority)
        {
            m_system_priority_list.push(a_evt);
        }else
        if(a_evt->m_flags & bree::context_event_flag_high_priority)
        {
            m_high_priority_list.push(a_evt);
        }
        else
        {
            m_event_list.push(a_evt);
        }
    }
    m_con_var.notify_one();

    return true;
}
namespace bree
{
namespace this_thread
{
thread_local bree_context_ptr context = nullptr;
}/// namespace this_thread
namespace context
{

void destroy_basic_context(bree_object_ptr)
{

}
BREE_PUBLIC bree_context_ptr context_new()
{
    bree_context_ptr l_context = std::make_shared<bree_context>();
    bree::context::global::register_context(l_context,&destroy_basic_context);
    return l_context;
}

BREE_PUBLIC void register_object(bree_context_ptr a_context,bree_object_ptr a_object,bree::object_unregister_callback a_cb)
{
    bree_context_register_event*l_evt = new bree_context_register_event(a_object,a_cb);
    a_context->push_event(l_evt);
}
BREE_PUBLIC void unregister_object(bree_context_ptr a_context,bree_object_ptr a_object)
{
    bree_context_unregister_event*l_evt = new bree_context_unregister_event(a_object);
    a_context->push_event(l_evt);
}

}/// namespace context


}/// namespace bree
