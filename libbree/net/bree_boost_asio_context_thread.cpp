#include <net/bree_boost_asio_context_thread.h>



namespace bree{
namespace net{
namespace context{

void destroy_asio_context_thread_callback(bree_object_ptr a_object){

bree::context::global::unregister_thread(a_object->cast<bree_context_thread>());
std::cout << "destroy asio context THREAD:" << a_object.use_count() << std::endl;
}
bree_context_ptr context_new(){
bree_asio_context_thread_ptr l_thread = std::make_shared<bree_asio_context_thread>();
bree_context_ptr l_context = l_thread->cast<bree_context>();
bree::context::global::register_context(l_context,&destroy_asio_context_thread_callback);
bree::context::global::register_thread(l_thread->cast<bree_context_thread>());
return l_context;
}
}/// context
}/// net
}/// bree
