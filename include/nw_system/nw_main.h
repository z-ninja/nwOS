#ifndef NW_MAIN_H
#define NW_MAIN_H

typedef struct _nw_event nw_event;
typedef boost::shared_ptr<nw_event> nw_event_ptr;

typedef struct _nw_interval_event nw_interval_event;
typedef boost::shared_ptr<nw_interval_event> nw_interval_event_ptr;

typedef struct _nw_service nw_service;
typedef boost::shared_ptr<nw_service> nw_service_ptr;
typedef struct _nw_timeout_event nw_timeout_event;
typedef boost::shared_ptr<nw_timeout_event> nw_timeout_event_ptr;

#define NW_SYSTEM_EXEC(c) nw_system_service_exec(c,__FILE__,__LINE__);
#define NW_SYSTEM_TIMEOUT(c,m) nw_system_service_timeout(c,m,__FILE__,__LINE__);
#define NW_SYSTEM_INTERVAL(c,m) nw_system_service_interval(c,m,__FILE__,__LINE__);



void nw_main_clear_timeouts(nw_service_ptr svr);
void nw_main_clear_timeout(uint64_t id);
void nw_main_add_timeout(nw_timeout_event_ptr tm);

/** exports */
 int nw_system_init(int &argc,char **&argv);
 int nw_system_service_run(int interval = 30);
 int nw_system_service_run_once();
 void nw_system_service_quit(int);
 void nw_system_service_exec(std::function<void()>,std::string, int);
 uint64_t nw_system_service_timeout(std::function<void()>,uint64_t,std::string, int);
 uint64_t nw_system_service_interval(std::function<bool()>,uint64_t,std::string, int);
 void nw_system_service_timeout_clear(uint64_t);
 void nw_system_service_interval_clear(uint64_t);
 void nw_system_service_push_event(nw_event_ptr&);
 bool nw_system_service_done();






#endif // NW_MAIN_H
