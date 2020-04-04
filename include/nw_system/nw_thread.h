#ifndef NW_THREAD_H
#define NW_THREAD_H
#include <functional>

typedef struct _nw_thread nw_thread;
typedef boost::shared_ptr<nw_thread> nw_thread_ptr;


struct _nw_thread : public _nw_object{

_nw_thread();
virtual~_nw_thread();
NW_OBJ_DECL(thread_ptr)
void run(std::function<int()>);
void stop(std::function<void(nw_thread_ptr)>cb=nullptr);
int join();
int get_exit_code();
int get_status();
virtual void destroy(std::string, int);
};
/** internal */
void nw_thread_join(nw_thread_ptr);

/** exports external */
nw_thread_ptr nw_thread_new();
bool nw_is_thread(nw_object_ptr ptr);
nw_thread_ptr nw_thread_from_object(nw_object_ptr);
nw_object_ptr nw_thread_to_object(nw_thread_ptr);
void nw_thread_run(nw_thread_ptr,std::function<int()>cb);
int nw_thread_get_status(nw_thread_ptr ptr);
int nw_thread_get_exit_code(nw_thread_ptr ptr);
void nw_thread_stop(nw_thread_ptr,std::function<void(nw_thread_ptr)>cb=nullptr);
void nw_thread_destroy(nw_thread_ptr,std::string,int);
bool nw_thread_set_join_handler(nw_thread_ptr,std::function<bool(nw_thread_ptr,int)>);
bool nw_thread_on_status_change(nw_thread_ptr,std::function<void(nw_thread_ptr,int)> );
#endif // NW_SIGNAL_H
