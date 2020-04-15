#ifndef BREE_CONTEXT_H
#define BREE_CONTEXT_H
#include <base/bree_object.h>

namespace bree
{
enum context_flags
{
    context_event_flag_system_priority      = 1 << 0,
    context_event_flag_high_priority        = 1 << 1,
    context_event_flag_post                 = 1 << 2,
    context_event_flag_register_object      = 1 << 3,
    context_event_flag_unregister_object    = 1 << 4,
    context_event_flag_quit                 = 1 << 5,
    context_event_flag_exit_requested       = 1 << 6,
    context_flags_state_closed              = 1 << 7,
    context_flags_main                      = 1 << 8,
};


typedef void (*object_unregister_callback)(bree_object_ptr);
void bree_init(int&a_argc,char**&a_argv,bree_context_ptr a_context = nullptr);
void bree_run();
void bree_exit(int);
int bree_exit_code();
int&get_argc();
char**&get_argv();
}/// namespace bree
class BREE_PUBLIC bree_context : public bree_object
{
    friend class bree::bree_context_manager;
public:
    BREE_OBJ_DEF(bree_context)
    bree_context();
    virtual ~bree_context();
    virtual void run();
    virtual size_t run_once();
    virtual void stop();
    virtual bool post(std::function<void()>,bool a_high_priority=false);
    virtual bool push_event(bree_event_ptr);
    virtual bool on_event(bree_event_ptr);
    virtual void exit(bree_context_ptr);
    virtual bool closed();
protected:
    bool m_running;
    std::mutex m_mutex;
    std::condition_variable m_con_var;
    std::queue<bree_event_ptr> m_event_list;
    std::queue<bree_event_ptr> m_high_priority_list;
    std::queue<bree_event_ptr> m_system_priority_list;
    std::map<bree_object_ptr,bree::object_unregister_callback> m_objects_list;
    bree_context_ptr m_parent_context;
};

namespace bree
{
namespace timer{
typedef void* timer_t;
timer_t timeout(bree_context_ptr,std::function<void()>,int64_t a_milliseconds);
timer_t interval(bree_context_ptr,std::function<bool()>,int64_t a_milliseconds);
void cancel(timer_t);
}/// namespace timer

namespace this_thread
{
extern thread_local bree_context_ptr context;
bree::timer::timer_t timeout(std::function<void()>,int64_t a_milliseconds);
bree::timer::timer_t interval(std::function<bool()>,int64_t a_milliseconds);
}/// namespace this_thread
namespace main_thread{
bree::timer::timer_t timeout(std::function<void()>,int64_t a_milliseconds);
bree::timer::timer_t interval(std::function<bool()>,int64_t a_milliseconds);
}
namespace context
{

bree_context_ptr context_new();
void register_object(bree_context_ptr,bree_object_ptr,bree::object_unregister_callback);
void unregister_object(bree_context_ptr,bree_object_ptr);
namespace global {
void register_context(bree_context_ptr,bree::object_unregister_callback);
void main_context_done(bree_context_ptr a_context);

}/// namespace global
}/// namespace context
}/// namespace bree



#endif // BREE_CONTEXT_H
