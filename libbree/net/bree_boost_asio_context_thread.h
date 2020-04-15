#ifndef BREE_BOST_ASIO_CONTEXT_THREAD_H
#define BREE_BOST_ASIO_CONTEXT_THREAD_H
#include <base/bree.h>
#include <boost/asio.hpp>
class bree_asio_context_thread;
/// quick net implementation test
typedef std::shared_ptr<bree_asio_context_thread> bree_asio_context_thread_ptr;
class bree_asio_context_thread :public bree_context_thread
{
protected:
    boost::asio::io_service ioc;
    virtual void work()
    {
        m_running = true;
        while(m_running)
        {
            ioc.run_one();
            while(ioc.poll_one()>0);
            run_once();
        }
        while(ioc.poll_one()>0);
    }
public:
    bree_asio_context_thread():bree_context_thread() {}
    BREE_OBJ_DEF(bree_asio_context_thread);
    virtual~bree_asio_context_thread() {}
    virtual bool push_event(bree_event_ptr a_event)
    {
       bool l_ret = bree_context::push_event(a_event);
       ioc.post([&]()->void{});
        return l_ret;
    }
    virtual bool post(std::function<void()>a_cb,bool a_high_priority)
    {
       bool l_ret = bree_context::post(a_cb,a_high_priority);
        ioc.post([&]()->void{});
        return l_ret;
    }
};
namespace bree{
namespace net{
namespace context{
bree_context_ptr context_new();
}/// context
}/// net
}/// bree

#endif // BREE_BOST_ASIO_CONTEXT_THREAD_H
