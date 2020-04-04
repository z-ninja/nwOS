#ifndef NW_EVENT_H
#define NW_EVENT_H

#include <nw_system/base.h>
#include <nw_system/nw_object.h>
typedef struct _nw_event nw_event;
typedef boost::shared_ptr<nw_event> nw_event_ptr;

typedef struct _nw_exec_event nw_exec_event;
typedef boost::shared_ptr<nw_exec_event> nw_exec_event_ptr;

typedef struct _nw_timeout_event nw_timeout_event;
typedef boost::shared_ptr<nw_timeout_event> nw_timeout_event_ptr;

typedef struct _nw_interval_event nw_interval_event;
typedef boost::shared_ptr<nw_interval_event> nw_interval_event_ptr;

typedef struct _nw_service nw_service;
typedef boost::shared_ptr<nw_service> nw_service_ptr;



struct NW_PUBLIC _nw_event : public _nw_object{

_nw_event(int,std::string,int);
virtual~_nw_event();
NW_OBJ_DECL(event_ptr)
virtual void do_action();
virtual nw_event_ptr to_event();
virtual uint64_t get_id();
int type;
std::string file;
int line;
};

struct _nw_exec_event : public _nw_event{

_nw_exec_event(std::function<void()>,std::string,int);
virtual~_nw_exec_event();
NW_OBJ_DECL(exec_event_ptr)
virtual nw_event_ptr to_event();
void do_action();
std::function<void()>cb;
};

struct _nw_timeout_event : public _nw_event{
public:
_nw_timeout_event(std::function<void()>,uint64_t milliseconds,nw_service_ptr svr,std::string,int);
virtual~_nw_timeout_event();
NW_OBJ_DECL(timeout_event_ptr)
virtual nw_event_ptr to_event();
virtual void do_action();
virtual uint64_t get_id();
std::function<void()>cb;
uint64_t id;
nw_service_ptr service;
};

struct _nw_interval_event : public _nw_timeout_event{

_nw_interval_event(std::function<bool()>,uint64_t milliseconds,nw_service_ptr,std::string,int);
virtual~_nw_interval_event();
NW_OBJ_DECL(interval_event_ptr)
virtual nw_event_ptr to_event();
uint64_t interval;
};




nw_exec_event_ptr nw_exec_event_new(std::function<void()>,std::string,int);
nw_timeout_event_ptr nw_timeout_event_new(nw_service_ptr,std::function<void()>,uint64_t milliseconds,std::string,int);
nw_interval_event_ptr nw_interval_event_new(nw_service_ptr,std::function<bool()>,uint64_t milliseconds,std::string,int);


/** externals */
void nw_event_do_action(nw_event_ptr ptr);

#endif // NW_EVENT_H
