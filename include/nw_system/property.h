#ifndef NW_PROPERTY_H
#define NW_PROPERTY_H
#include <map>
#include <boost/any.hpp>
#include <nw_system/export.h>

typedef std::map<std::string,boost::any> properties;
template <class T>
bool nw_property_set(properties&property_list,std::string name,T val){
auto v = property_list.find(name);
boost::any value = val;
    if(v != property_list.end())
    {
        if(v->second.type() == value.type())
        {
            v->second = value;
            return true;
        }
    }

return false;
}
NW_PUBLIC bool nw_property_remove(properties&property_list,std::string name);
bool nw_property_get(properties&property_list,std::string name,boost::any&value);
template<class T>
bool nw_property_extract(boost::any&value_any,T*value){

try{
(*value) = boost::any_cast<T>(value_any);
return true;
}catch(boost::bad_any_cast &e){


}
return false;
}
template<class T>
bool nw_property_get_extract(properties&property_list,std::string name,T*value){

boost::any value_any;
if(nw_property_get(property_list,name,value_any)){

if(nw_property_extract<T>(value_any,value))
return true;


}
return false;
}



template<class T>
bool nw_property_init(properties&property_list,std::string name,T value){
auto v = property_list.find(name);
    if(v != property_list.end())
    {
       return false;
    }
    boost::any value_eny = value;
    property_list.insert(std::pair<std::string,boost::any>(name,value_eny));
    return true;
}


#endif // NW_PROPERTY_H
