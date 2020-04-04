#ifndef NW_SYSTEM_H
#define NW_SYSTEM_H
#include <boost/shared_ptr.hpp>
#include <vector>
#include <nw_system/property.h>
#include <nw_system/nw_assert.h>
#include <stdint.h>
typedef struct _nw_object nw_object;
typedef boost::shared_ptr<nw_object> nw_object_ptr;


typedef struct _nw_service nw_service;
typedef boost::shared_ptr<nw_service> nw_service_ptr;


typedef struct _nw_event nw_event;
typedef boost::shared_ptr<nw_event> nw_event_ptr;

typedef struct _nw_exec_event nw_exec_event;
typedef boost::shared_ptr<nw_exec_event> nw_exec_event_ptr;

typedef struct _nw_timeout_event nw_timeout_event;
typedef boost::shared_ptr<nw_timeout_event> nw_timeout_event_ptr;

typedef struct _nw_interval_event nw_interval_event;
typedef boost::shared_ptr<nw_interval_event> nw_interval_event_ptr;

typedef struct _nw_thread nw_thread;
typedef boost::shared_ptr<nw_thread> nw_thread_ptr;

typedef struct _nw_thread_service nw_thread_service;
typedef boost::shared_ptr<nw_thread_service> nw_thread_service_ptr;







#define NW_SYSTEM_EXEC(c) nw_system_service_exec(c,__FILE__,__LINE__);
#define NW_SYSTEM_TIMEOUT(c,m) nw_system_service_timeout(c,m,__FILE__,__LINE__);
#define NW_SYSTEM_INTERVAL(c,m) nw_system_service_interval(c,m,__FILE__,__LINE__);


#define NW_SERVICE_EXEC(l,c) nw_service_exec(l,c,__FILE__,__LINE__)
#define NW_SERVICE_TIMEOUT(l,c,m) nw_service_set_timeout(l,c,m,__FILE__,__LINE__)
#define NW_SERVICE_INTERVAL(l,c,m)nw_service_set_interval(l,c,m,__FILE__,__LINE__)
#define NW_SERVICE(l)nw_service_from_object(l)


#define NW_THREAD_SERVICE_EXEC(l,c) nw_thread_service_exec(l,c,__FILE__,__LINE__)
#define NW_THREAD_SERVICE_TIMEOUT(l,c,m) nw_thread_service_set_timeout(l,c,m,__FILE__,__LINE__)
#define NW_THREAD_SERVICE_INTERVAL(l,c,m)nw_thread_service_set_interval(l,c,m,__FILE__,__LINE__)
#define NW_THREAD_SERVICE(l)nw_thread_service_from_object(l)


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
void nw_system_service_quit(int reason);

/** nw object */
void nw_object_ref_decrement(std::string type_name);
void nw_object_ref_increment(std::string type_name);
void nw_object_show_ref_stat();

void nw_object_unref_real(nw_object_ptr ptr);

void nw_object_ref(nw_object_ptr);/// ok
void nw_object_unref(nw_object_ptr);/// ok
std::string nw_object_get_type_string(nw_object_ptr); /// ok
bool nw_object_init_user_data(nw_object_ptr,std::function<void(properties&)>);
bool nw_object_get_user_data(nw_object_ptr,std::function<void(properties&)>);
bool nw_object_get_property_int(nw_object_ptr ptr,std::string prop_name,int*value); /// ok
bool nw_object_set_property_int(nw_object_ptr ptr,std::string prop_name,int value); /// ok
bool nw_object_get_property_bool(nw_object_ptr ptr,std::string prop_name,bool*value); /// ok
bool nw_object_set_property_bool(nw_object_ptr ptr,std::string prop_name,bool value); /// ok
bool nw_object_get_property_string(nw_object_ptr ptr,std::string prop_name,std::string&value); /// ok
bool nw_object_set_property_string(nw_object_ptr ptr,std::string prop_name,std::string value); /// ok
std::string nw_object_get_last_lock_file(nw_object_ptr ptr);// ok
int nw_object_get_last_lock_line(nw_object_ptr ptr); // ok

nw_service_ptr nw_service_new(std::string); /// ok
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



void nw_event_do_action(nw_event_ptr ptr);



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


uint64_t nw_get_unique_id();

uint16_t nw_swap_uint16( uint16_t val);
int16_t nw_swap_int16( int16_t val);
uint32_t nw_swap_uint32( uint32_t val);
int32_t nw_swap_int32( int32_t val);
int64_t nw_swap_int64( int64_t val);
uint64_t nw_swap_uint64( uint64_t val);
float nw_swap_float(float val);
double nw_swap_double(double val);


uint16_t nw_readUint16LE( uint16_t val );
uint16_t nw_writeUint16LE( uint16_t val );
uint16_t nw_readUint16BE( uint16_t val );
uint16_t nw_writeUint16BE( uint16_t val );


int16_t nw_readInt16LE( int16_t val );
int16_t nw_writeInt16LE( int16_t val );
int16_t nw_readInt16BE( int16_t val );
int16_t nw_writeInt16BE( int16_t val );

uint32_t nw_readUint32LE( uint32_t val );
uint32_t nw_writeUint32LE( uint32_t val );
uint32_t nw_readUint32BE( uint32_t val );
uint32_t nw_writeUint32BE( uint32_t val );


int32_t nw_readInt32LE( int32_t val );
int32_t nw_writeInt32LE( int32_t val );
int32_t nw_readInt32BE( int32_t val );
int32_t nw_writeInt32BE( int32_t val );


uint64_t nw_readUint64LE( uint64_t val );
uint64_t nw_writeUint64LE( uint64_t val );
uint64_t nw_readUint64BE( uint64_t val );
uint64_t nw_writeUint64BE( uint64_t val );

int64_t nw_readInt64LE( int64_t val );
int64_t nw_writeInt64LE( int64_t val );
int64_t nw_readInt64BE( int64_t val );
int64_t nw_writeInt64BE( int64_t val );


float nw_readFloatLE( float val );
float nw_writeFloatLE( float val );
float nw_readFloatBE( float val );
float nw_writeFloatBE( float val );


double nw_readDoubleLE( double val );
double nw_writeDoubleLE( double val );
double nw_readDoubleBE( double val );
double nw_writeDoubleBE( double val );

#endif // NW_SYSTEM_H
