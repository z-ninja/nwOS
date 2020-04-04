#ifndef NW_MACROS_H
#define NW_MACROS_H

#define NW_OBJ_DEF(name, ptr,type_name) nw_##ptr _nw_##name::shared_from_this(){\
return shared_from(this);\
}\
nw_object_ptr _nw_##name::to_object(){\
return shared_from_this();\
}\
std::string _nw_##name::get_type(){\
return #type_name;\
}
#define NW_OBJ_DECL(ptr) nw_##ptr shared_from_this();\
virtual nw_object_ptr to_object();\
virtual std::string get_type();


#define NW_OBJ_DESTROY_PROTECT  {\
    boost::any closed_any;\
    std::lock_guard<std::mutex> guard(property_mutex);\
    if(nw_property_get(property_list,"object-destroyed",closed_any)){\
    bool isclosed = false;\
    try{\
     isclosed = boost::any_cast<bool>(closed_any);\
    }catch(boost::bad_any_cast &e){\
    std::cout << CONSOLE_SYSTEM_ERROR <<"bad cast destroy any: " << __FILE__ << ":" << __LINE__ << " - " << e.what() << std::endl;\
    return;\
    }\
    if(isclosed){\
    std::cout << CONSOLE_SYSTEM_WARN<<"CALLING DESTROY MULTIPLE TIMES IS NOT AN OPTION " << CONSOLE_COLOR_PURPLE(get_type()) << "-"<< file << ":" << line << std::endl;\
    describe();\
    return;\
    }\
    nw_property_set<bool>(property_list,"object-destroyed",true);\
    }\
    }

#define NW_OBJECT_RETURN_BOOL_IS(x,p) try\
    {\
        return (dynamic_cast<_nw_ ##x *>(p.get())!= 0);\
    }\
    catch (std::exception& e)\
    {\
        std::cout << "Exception dynamic_cast: " << __FILE__ << ":" << __LINE__<< " - " << e.what();\
    }\
    return false;

#define NW_NULL_OBJECT_WARNING std::cout <<CONSOLE_SYSTEM_WARN << "null object supplied: " << CONSOLE_COLOR_PURPLE(std::string(__FUNCTION__)) << std::endl;
#endif // NW_MACROS_H
