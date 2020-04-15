#ifndef BREE_GTK_CONTEXT_H
#define BREE_GTK_CONTEXT_H
#include <base/bree.h>
#include <gtk/gtk.h>
class bree_gtk_context;
/// quick ui test implementation
typedef std::shared_ptr<bree_gtk_context> bree_gtk_context_ptr;
class bree_gtk_context : public bree_context
{
public:
    BREE_OBJ_DEF(bree_gtk_context);
    bree_gtk_context():bree_context() {}
    virtual~bree_gtk_context() {}
    virtual void run()
    {
        m_running = true;
        bree::this_thread::context = shared_from_this();
        while(m_running)
        {
            run_once();
            while(g_main_context_iteration (NULL, m_running));
        }
        bree::this_thread::context = nullptr;
    }

    virtual bool push_event(bree_event_ptr a_event)
    {
       bool l_ret = bree_context::push_event(a_event);
        g_main_context_wakeup(nullptr);
        return l_ret;
    }
    virtual bool post(std::function<void()>a_cb,bool a_high_priority)
    {
       bool l_ret = bree_context::post(a_cb,a_high_priority);
        g_main_context_wakeup(nullptr);
        return l_ret;
    }
};

namespace bree {
namespace ui{
namespace gtk{
namespace context{
bree_context_ptr context_new();
}/// context
}/// gtk
}/// ui
}/// bree

#endif // BREE_GTK_CONTEXT_H
