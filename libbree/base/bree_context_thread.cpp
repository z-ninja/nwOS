#include <base/bree.h>


namespace bree{



namespace context {

void destroy_context_thread_callback(bree_object_ptr a_object){

bree::context::global::unregister_thread(a_object->cast<bree_context_thread>());
}
bree_context_ptr context_thread_new(){
bree_context_thread_ptr l_thread = std::make_shared<bree_context_thread>();
bree::context::global::register_context(l_thread->cast<bree_context>(),&destroy_context_thread_callback);
bree::context::global::register_thread(l_thread);
return l_thread;
}

}/// namespace context
}/// namespace bree
