#ifndef NW_SERVICE_H
#define NW_SERVICE_H

typedef struct _nw_service nw_service;
typedef boost::shared_ptr<nw_service> nw_service_ptr;

typedef struct _nw_event nw_event;
typedef boost::shared_ptr<nw_event> nw_event_ptr;

typedef struct _nw_timeout_event nw_timeout_event;
typedef boost::shared_ptr<nw_timeout_event> nw_timeout_event_ptr;

typedef struct _nw_interval_event nw_interval_event;
typedef boost::shared_ptr<nw_interval_event> nw_interval_event_ptr;

struct _nw_service : public _nw_object{

_nw_service(std::string);
virtual~_nw_service();
NW_OBJ_DECL(service_ptr)
int run(int interval=30);
int run_once();
int run_one_only();
void stop(int,std::function<void(int)>cb = nullptr);
void exec(std::function<void()>,std::string, int);
uint64_t set_timeout(std::function<void()>,uint64_t,std::string, int);
uint64_t set_interval(std::function<bool()>,uint64_t,std::string, int);
void set_interval(nw_interval_event_ptr evt);
void clear_timeout(uint64_t);
void clear_interval(uint64_t);
void push_event(nw_event_ptr&);
void set_persistent(bool);
bool get_persistent();
virtual void describe();
virtual void destroy(std::string, int);
std::string name;
uint64_t id;
bool stop_request;
bool persistent;
bool normal_run;
bool finished;
int exit_code;
std::queue<nw_event_ptr> event_list;
std::vector<nw_event_ptr> ready_handlers_list;
std::map<uint64_t,nw_timeout_event_ptr> timeout_list;
std::vector<uint64_t> timeout_clear_request;
std::mutex prop_mutex;
std::mutex event_mutex;
std::condition_variable event_cond;
std::function<void(int)> stop_callback;
private:
void set_run_mode(bool);
bool get_run_mode();
void set_finished(bool);
bool get_finished();
};

#define NW_SERVICE_EXEC(l,c) nw_service_exec(l,c,__FILE__,__LINE__)
#define NW_SERVICE_TIMEOUT(l,c,m) nw_service_set_timeout(l,c,m,__FILE__,__LINE__)
#define NW_SERVICE_INTERVAL(l,c,m)nw_service_set_interval(l,c,m,__FILE__,__LINE__)
#define NW_SERVICE(l)nw_service_from_object(l)
/** exports */
  nw_service_ptr nw_service_new(std::string);
  bool nw_is_service(nw_object_ptr);
  nw_service_ptr nw_service_from_object(nw_object_ptr);
  nw_object_ptr nw_service_to_object(nw_service_ptr);

  int nw_service_run(nw_service_ptr,int interval=30);
  int nw_service_run_once(nw_service_ptr);
  int nw_service_run_one_only(nw_service_ptr);
  void nw_service_stop(nw_service_ptr,int,std::function<void(int)>cb = nullptr);
  void nw_service_exec(nw_service_ptr,std::function<void()>,std::string, int);
  uint64_t nw_service_set_timeout(nw_service_ptr,std::function<void()>,uint64_t,std::string, int);
  uint64_t nw_service_set_interval(nw_service_ptr,std::function<bool()>,uint64_t,std::string, int);
  void nw_service_set_interval(nw_service_ptr,nw_interval_event_ptr);
  void nw_service_clear_timeout(nw_service_ptr,uint64_t);
  void nw_service_clear_interval(nw_service_ptr,uint64_t);
  void nw_service_push_event(nw_service_ptr,nw_event_ptr&);
  void nw_service_set_persistent(nw_service_ptr,bool);
  bool nw_service_get_persistent(nw_service_ptr);





#endif // nw_service_H
