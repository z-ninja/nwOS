#include <vector>
#include <boost/bind.hpp>
#include <nw_system/base.h>
#include <nw_system/nw_object.h>
#include <nw_system/nw_main.h>

NW_OBJ_DEF(object,object_ptr,[nw_object])

std::mutex object_ref_count_mutex;
std::map<std::string,std::pair<int,int>> ref_count_list;
NW_PUBLIC void nw_object_ref_increment(std::string type_name)
{
    std::lock_guard<std::mutex> guard(object_ref_count_mutex);
    auto r = ref_count_list.find(type_name);
    if(r == ref_count_list.end())
    {
        ref_count_list.insert(std::pair<std::string,std::pair<int,int>>(type_name,std::pair<int,int>(1,1)));
    }
    else
    {
        r->second.first += 1;
        r->second.second += 1;
    }
}
NW_PUBLIC void nw_object_ref_decrement(std::string type_name)
{
    std::lock_guard<std::mutex> guard(object_ref_count_mutex);
    auto r = ref_count_list.find(type_name);
    if(r != ref_count_list.end())
    {
        r->second.first  -= 1;
    }else {
    std::cout <<CONSOLE_SYSTEM_WARN<< "object un handled ref decrement " << CONSOLE_COLOR_PURPLE(type_name) << std::endl;
    }
}
bool _nw_object::is_reffed(){
bool unreffed = false;
        property_lock(__FILE__,__LINE__);
        if(!nw_property_get_extract<bool>
        (property_list,"object-reffed",&unreffed))
        {
            std::cout << "unable to extract object-reffed cb" << std::endl;
        }
        property_unlock();
return unreffed;
}
_nw_object::_nw_object():nw_object_base(),user_data_destructor(nullptr),last_lock_file(""),last_lock_line(0),
property_list(),user_data_list(),property_mutex(),user_data_mutex()
{
    nw_property_init<bool>(property_list,"object-destroyed",false);
    nw_property_init<bool>(property_list,"object-reffed",false);
}
_nw_object::~_nw_object()
{
    property_lock(__FILE__,__LINE__);;
    auto p = property_list.begin();
    while(p != property_list.end())
    {
        property_list.erase(p);
        p = property_list.begin();
    }
    property_unlock();
    user_data_mutex.lock();
    if(user_data_destructor&&!nw_system_service_done()){
       user_data_destructor(user_data_list);
    }
     user_data_destructor = nullptr;
    auto u = user_data_list.begin();
    while(u != user_data_list.end()){
        user_data_list.erase(u);
        u = user_data_list.begin();
    }
    user_data_mutex.unlock();
}
bool _nw_object::init_user_data(std::function<void(properties&)>cb){
std::lock_guard<std::mutex> guard(user_data_mutex);
if(user_data_destructor == nullptr){
user_data_destructor = cb;
return true;
}

return false;
}
void _nw_object::uninit_init_user_data(){
user_data_mutex.lock();
if(user_data_destructor)
user_data_destructor(user_data_list);
user_data_destructor = nullptr;
user_data_mutex.unlock();
}
bool _nw_object::get_user_data(std::function<void(properties&)>cb){
std::lock_guard<std::mutex> guard(user_data_mutex);
if(user_data_destructor){
   if(cb)
cb(user_data_list);
return true;
}
return false;
}
void _nw_object::describe(){

}
void _nw_object::destroy(std::string file, int line)
{
NW_OBJ_DESTROY_PROTECT
}

NW_PUBLIC std::unique_lock<std::mutex> _nw_object::get_property_unique_lock(){

std::unique_lock<std::mutex> l(property_mutex,std::defer_lock);
#ifdef NW_WINDOWS
return l;
#elif defined NW_LINUX
return std::move(l);
#else
return l;
#endif // NW_WINDOWS

}
NW_PUBLIC void _nw_object::property_lock(std::string file,int line)
{
    property_mutex.lock();
last_lock_file = file;
last_lock_line = line;
}
NW_PUBLIC void _nw_object::property_unlock()
{
    property_mutex.unlock();
}


NW_PUBLIC void nw_object_show_ref_stat()
{
    std::lock_guard<std::mutex> g(object_ref_count_mutex);
    std::cout <<CONSOLE_SYSTEM_INFO<< "ref stat"<< std::endl;
    for(auto r=ref_count_list.begin(); r!= ref_count_list.end(); r++)
    {
        std::cout << CONSOLE_COLOR_PURPLE(r->first) << "\t-\t" << r->second.first << ":" << r->second.second  << std::endl;
    }
}
std::vector<nw_object_ptr> nw_object_list;
std::mutex object_list_mutex;
std::function<void()> exit_handler = nullptr;
void nw_object_clean_up(std::string file,int line)
{
    #ifdef NW_DEBUG
    std::cout << "nw_object_clean_up: " << file << ":" << line << std::endl;
    #endif
    std::lock_guard<std::mutex> guard(object_list_mutex);
    //std::cout << "nw_object_list.size(): " << nw_object_list.size() << std::endl;
    auto o = nw_object_list.begin();
    while(o != nw_object_list.end())
    {
        (*o)->destroy(__FILE__,__LINE__);
        nw_object_list.erase(o++);
    }


}

void nw_object_destroy_all()
{
    std::lock_guard<std::mutex> guard(object_list_mutex);
    auto o = nw_object_list.begin();
    while(o != nw_object_list.end())
    {
        (*o)->destroy(__FILE__,__LINE__);
        o++;
    }
}

NW_PUBLIC void nw_object_unref_real(nw_object_ptr ptr)
{
    if(!ptr)
        NW_NULL_OBJECT_WARNING
    std::lock_guard<std::mutex> guard(object_list_mutex);
    auto o = std::find(nw_object_list.begin(),nw_object_list.end(),ptr);
    if(o != nw_object_list.end())
    {
        #ifdef NW_DEBUG
        std::cout << CONSOLE_SYSTEM_DEBUG << "unref object" << CONSOLE_COLOR_PURPLE(ptr->get_type()) << std::endl;
        #endif
        ptr->property_lock(__FILE__,__LINE__);;
        nw_property_set<bool>(ptr->property_list,"object-reffed",false);
        ptr->property_unlock();
        if(!nw_system_service_done())
        (*o)->uninit_init_user_data();

        nw_object_list.erase(o);

    }else std::cout << CONSOLE_SYSTEM_WARN << "traying to unref non reffed object: " << CONSOLE_COLOR_PURPLE(ptr->get_type()) << std::endl;

    if(exit_handler && nw_object_list.size()==0)
    {
       std::function<void()> eH = exit_handler;
       NW_SYSTEM_EXEC(([&,eH]()->void{
        eH();
        }));
        exit_handler = nullptr;
    }
}
void nw_object_set_exit_handler(std::function<void()>cb)
{
    std::lock_guard<std::mutex> guard(object_list_mutex);
    exit_handler = cb;
    if(exit_handler&&nw_object_list.size() == 0){
            exit_handler();
            exit_handler = nullptr;
    }else {

    NW_SYSTEM_TIMEOUT(([&]()->void{
     std::lock_guard<std::mutex> guard(object_list_mutex);
    if(exit_handler){
        if(nw_object_list.size()>0){
    std::cout << CONSOLE_SYSTEM_WARN<< nw_object_list.size() << " objects are not responding on exit event, list of object types shown bellow:" << std::endl;
    auto o = nw_object_list.begin();
    while(o != nw_object_list.end()){
    std::cout << CONSOLE_COLOR_PURPLE((*o)->get_type())  << std::endl;
    (*o)->describe();
    nw_object_list.erase(o);
    o = nw_object_list.begin();
    }
    }
            exit_handler();
            exit_handler = nullptr;
    }
    }),10000);
    }
}
/** exports */
NW_PUBLIC void nw_object_ref(nw_object_ptr ptr)
{
    if(!ptr)
    NW_NULL_OBJECT_WARNING
    std::lock_guard<std::mutex> guard(object_list_mutex);
    if(std::find(nw_object_list.begin(),nw_object_list.end(),ptr)== nw_object_list.end())
    {
        #ifdef NW_DEBUG
        std::cout << CONSOLE_SYSTEM_DEBUG <<"refing object: " << CONSOLE_COLOR_PURPLE(ptr->get_type()) << std::endl;
        ptr->describe();
        #endif
       //
         ptr->property_lock(__FILE__,__LINE__);;
        nw_property_set<bool>(ptr->property_list,"object-reffed",true);
        ptr->property_unlock();
        nw_object_list.push_back(ptr);
    }else std::cout << CONSOLE_SYSTEM_WARN << "trying to ref already reffed object: " << CONSOLE_COLOR_PURPLE(ptr->get_type())<< std::endl;
}

NW_PUBLIC void nw_object_unref(nw_object_ptr ptr)
{
    if(!ptr)
    NW_NULL_OBJECT_WARNING
    if(nw_system_service_done())
    nw_object_unref_real(ptr);
      else
     NW_SYSTEM_EXEC(boost::bind(&nw_object_unref_real,ptr));
}
NW_PUBLIC std::string nw_object_get_type_string(nw_object_ptr ptr)
{
    if(ptr)
    {
        return ptr->get_type();
    }
    else {
     NW_NULL_OBJECT_WARNING
        return "[null]";
    }
}

NW_PUBLIC bool nw_object_init_user_data(nw_object_ptr ptr,std::function<void(properties&)> cb){
if(ptr)
    {
    return ptr->init_user_data(cb);
    }else NW_NULL_OBJECT_WARNING
return false;
}
NW_PUBLIC bool nw_object_get_user_data(nw_object_ptr ptr,std::function<void(properties&)>cb){
if(ptr)
    {
        return ptr->get_user_data(cb);
    }else NW_NULL_OBJECT_WARNING
    return false;
}


NW_PUBLIC  bool nw_object_get_property_bool(nw_object_ptr ptr,std::string prop_name,bool*value)
{
    if(!ptr||!value){
     NW_NULL_OBJECT_WARNING
        return false;
    }
    std::lock_guard<std::mutex> guard(ptr->property_mutex);
    boost::any value_any;
    if(nw_property_get(ptr->property_list,prop_name,value_any))
        return nw_property_extract<bool>(value_any,value);
    return false;
}
NW_PUBLIC  bool nw_object_set_property_bool(nw_object_ptr ptr,std::string prop_name,bool value)
{
    if(!ptr){
     NW_NULL_OBJECT_WARNING
        return false;
    }
    std::lock_guard<std::mutex> guard(ptr->property_mutex);
    return nw_property_set<bool>(ptr->property_list,prop_name,value);
}
NW_PUBLIC  bool nw_object_get_property_int(nw_object_ptr ptr,std::string prop_name,int*value)
{
    if(!ptr||!value){
    NW_NULL_OBJECT_WARNING
        return false;
    }
    std::lock_guard<std::mutex> guard(ptr->property_mutex);
    boost::any value_any;
    if(nw_property_get(ptr->property_list,prop_name,value_any))
        return nw_property_extract<int>(value_any,value);
    return false;
}
NW_PUBLIC  bool nw_object_set_property_int(nw_object_ptr ptr,std::string prop_name,int value)
{
    if(!ptr){
     NW_NULL_OBJECT_WARNING
        return false;
    }
    std::lock_guard<std::mutex> guard(ptr->property_mutex);
    return nw_property_set<int>(ptr->property_list,prop_name,value);
}
NW_PUBLIC bool nw_object_get_property_string(nw_object_ptr ptr,std::string prop_name,std::string&value)
{

    if(!ptr){
     NW_NULL_OBJECT_WARNING
        return false;
    }
    std::lock_guard<std::mutex> guard(ptr->property_mutex);
    boost::any value_any;
    if(nw_property_get(ptr->property_list,prop_name,value_any))
        return nw_property_extract<std::string>(value_any,&value);
    return false;
}
NW_PUBLIC  bool nw_object_set_property_string(nw_object_ptr ptr,std::string prop_name,std::string value)
{
    if(!ptr){
        NW_NULL_OBJECT_WARNING
        return false;
    }
    std::lock_guard<std::mutex> guard(ptr->property_mutex);
    return nw_property_set<std::string>(ptr->property_list,prop_name,value);
}

NW_PUBLIC std::string nw_object_get_last_lock_file(nw_object_ptr ptr){
if(!ptr){
        NW_NULL_OBJECT_WARNING
        return "";
    }
    return ptr->last_lock_file;
}
NW_PUBLIC int nw_object_get_last_lock_line(nw_object_ptr ptr){
if(!ptr){
        NW_NULL_OBJECT_WARNING
        return 0;
    }
    return ptr->last_lock_line;
}
