#ifndef BREE_CONTEXT_THREAD_H
#define BREE_CONTEXT_THREAD_H
#include <base/bree_context.h>
#include <thread>
class bree_context_thread;
typedef std::shared_ptr<bree_context_thread> bree_context_thread_ptr;
class bree_context_thread : public bree_context
{
        friend class bree::bree_context_manager;

protected:
    std::thread m_thread;
    void work_wrapper(){
    bree_context_ptr l_me = nullptr;
    do{
            try{
    l_me= shared_from_this();/// I get exception here bad weak ptr, I would like to understand theory behind
            }catch(std::exception&ex){
            std::cout << "WARNING: exception in thread construction" << ex.what() << std::endl;
            }
    }while(l_me == nullptr);
        bree::this_thread::context = l_me;
        work();
        bree::this_thread::context = nullptr;
    }
    virtual void work()
    {
        bree_context::run();
    }
public:
    BREE_OBJ_DEF(bree_context_thread);
    bree_context_thread():bree_context(),m_thread(std::bind(&bree_context_thread::work_wrapper,this))
    {

    }
    virtual ~bree_context_thread()
    {
    }
    virtual void run() {}

};

namespace bree
{

namespace context
{
namespace global {
void register_thread(bree_context_thread_ptr);
void unregister_thread(bree_context_thread_ptr);
}/// namespace global
bree_context_ptr context_thread_new();

}/// namespace context
}/// namespace bree

#endif // BREE_CONTEXT_THREAD_H
