#ifndef BREE_EVENT_H
#define BREE_EVENT_H

class bree_context;
typedef std::shared_ptr<bree_context> bree_context_ptr;
struct bree_event{
int m_flags;
bree_context_ptr m_source;
bree_context_ptr m_dest;
bree_event(int a_flags,bree_context_ptr a_dest);
bree_event(int a_flags);
virtual~bree_event(){}
};
typedef bree_event* bree_event_ptr;



#endif // BREE_EVENT_H
