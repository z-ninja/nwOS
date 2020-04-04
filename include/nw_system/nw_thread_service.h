#ifndef NW_THREAD_SERVICE_H
#define NW_THREAD_SERVICE_H

typedef struct _nw_thread_service nw_thread_service;
typedef boost::shared_ptr<nw_thread_service> nw_thread_service_ptr;

typedef struct _nw_thread nw_thread;
typedef boost::shared_ptr<nw_thread> nw_thread_ptr;

typedef struct _nw_service nw_service;
typedef boost::shared_ptr<nw_service> nw_service_ptr;


struct _nw_thread_service : public _nw_object{

_nw_thread_service(std::string);
virtual~_nw_thread_service();
NW_OBJ_DECL(thread_service_ptr)
int run_once();
int run_one_only();
void stop(std::function<void(nw_thread_service_ptr)>);
void exec(std::function<void()>,std::string, int);
uint64_t set_timeout(std::function<void()>,uint64_t,std::string, int);
uint64_t set_interval(std::function<bool()>,uint64_t,std::string, int);
void clear_timeout(uint64_t);
void clear_interval(uint64_t);
virtual void destroy(std::string,int);
virtual void on_finished(std::function<void(nw_thread_service_ptr,int)>);
virtual void set_join_handler(std::function<bool(nw_thread_service_ptr,int)>);
nw_thread_ptr mythread;
nw_service_ptr myservice;
};

/** externals */
#define NW_THREAD_SERVICE_EXEC(l,c) nw_thread_service_exec(l,c,__FILE__,__LINE__)
#define NW_THREAD_SERVICE_TIMEOUT(l,c,m) nw_thread_service_set_timeout(l,c,m,__FILE__,__LINE__)
#define NW_THREAD_SERVICE_INTERVAL(l,c,m)nw_thread_service_set_interval(l,c,m,__FILE__,__LINE__)
#define NW_THREAD_SERVICE(l)nw_thread_service_from_object(l)


nw_thread_service_ptr nw_thread_service_new(std::string);

bool nw_is_thread_service(nw_object_ptr);
nw_thread_service_ptr nw_thread_service_from_object(nw_object_ptr);
nw_object_ptr nw_thread_service_to_object(nw_thread_service_ptr);


void nw_thread_service_destroy(nw_thread_service_ptr,std::string,int);
int nw_thread_service_run_once(nw_thread_service_ptr ptr);
int nw_thread_service_run_one_only(nw_thread_service_ptr ptr);
void nw_thread_service_stop(nw_thread_service_ptr ptr,std::function<void(nw_thread_service_ptr)>cb);
void nw_thread_service_exec(nw_thread_service_ptr ptr,std::function<void()>cb,std::string source, int line);
uint64_t nw_thread_service_set_timeout(nw_thread_service_ptr ptr,std::function<void()>cb,uint64_t milliseconds,std::string source, int line);
uint64_t nw_thread_service_set_interval(nw_thread_service_ptr ptr,std::function<bool()>cb,uint64_t milliseconds,std::string source, int line);
void nw_thread_service_clear_timeout(nw_thread_service_ptr ptr,uint64_t id);
void nw_thread_service_clear_interval(nw_thread_service_ptr ptr,uint64_t id);
void nw_thread_service_on_finished(nw_thread_service_ptr,std::function<void(nw_thread_service_ptr,int)>);
void nw_thread_service_set_join_handler(nw_thread_service_ptr,std::function<bool(nw_thread_service_ptr,int)>);
#endif // NW_thread_service_H

