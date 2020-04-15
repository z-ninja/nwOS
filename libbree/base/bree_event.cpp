#include <base/bree.h>


bree_event::bree_event(int a_flags):m_flags(a_flags),m_source(bree::this_thread::context),m_dest(nullptr){}
bree_event::bree_event(int a_flags,bree_context_ptr a_dest):m_flags(a_flags),m_source(bree::this_thread::context),m_dest(a_dest){}

