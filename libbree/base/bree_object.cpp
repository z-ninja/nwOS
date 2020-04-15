#include <base/bree_context.h>



bree_object::bree_object():bree_object_base(),m_flags(0){
}
bree_object::~bree_object(){
}
void bree_object::exit(bree_context_ptr a_context){
bree::context::unregister_object(a_context,shared_from_this());
}

bool bree_object::on_event(bree_event_ptr){
return false;
}
