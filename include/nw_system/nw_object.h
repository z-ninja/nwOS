#ifndef NW_NW_H
#define NW_NW_H
#include <nw_system/property.h>
#ifdef NW_MAC_OS_X
#include <iostream>
#endif
#include <mutex>
typedef struct _nw_object nw_object;
typedef boost::shared_ptr<nw_object> nw_object_ptr;

struct NW_PUBLIC _nw_object : public nw_object_base{
_nw_object();
virtual~_nw_object();
NW_OBJ_DECL(object_ptr)
virtual void destroy(std::string, int);
virtual void describe();
void property_lock(std::string file,int line);
void property_unlock();
std::unique_lock<std::mutex> get_property_unique_lock();
bool init_user_data(std::function<void(properties&)>);
bool get_user_data(std::function<void(properties&)>);
void uninit_init_user_data();
bool is_reffed();
std::function<void(properties&)> user_data_destructor;
std::string last_lock_file;
int last_lock_line;
properties property_list;
properties user_data_list;
std::mutex property_mutex;
std::mutex user_data_mutex;

};
void nw_object_ref_decrement(std::string type_name);
void nw_object_ref_increment(std::string type_name);
void nw_object_show_ref_stat();

void nw_object_clean_up(std::string file,int line);
void nw_object_destroy_all();
void nw_object_unref_real(nw_object_ptr ptr);
void nw_object_set_exit_handler(std::function<void()>cb);
/** exports */
void nw_object_ref(nw_object_ptr);
void nw_object_unref(nw_object_ptr);
std::string nw_object_get_type_string(nw_object_ptr);
bool nw_object_init_user_data(nw_object_ptr,std::function<void(properties&)>);
bool nw_object_get_user_data(nw_object_ptr,std::function<void(properties&)>);
bool nw_object_get_property_int(nw_object_ptr ptr,std::string prop_name,int*value);
bool nw_object_set_property_int(nw_object_ptr ptr,std::string prop_name,int value);
bool nw_object_get_property_bool(nw_object_ptr ptr,std::string prop_name,bool*value);
bool nw_object_set_property_bool(nw_object_ptr ptr,std::string prop_name,bool value);
bool nw_object_get_property_string(nw_object_ptr ptr,std::string prop_name,std::string&value);
bool nw_object_set_property_string(nw_object_ptr ptr,std::string prop_name,std::string value);
std::string nw_object_get_last_lock_file(nw_object_ptr ptr);
int nw_object_get_last_lock_line(nw_object_ptr ptr);




#endif // NW_NW_H
