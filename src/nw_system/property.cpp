#include <nw_system/property.h>
#include <nw_system/export.h>

NW_PUBLIC bool nw_property_remove(properties&property_list,std::string name){
auto v = property_list.find(name);
    if(v != property_list.end())
    {
        property_list.erase(v);
        return true;
    }
return false;
}
NW_PUBLIC bool nw_property_get(properties&property_list,std::string name,boost::any&value){

auto v = property_list.find(name);
    if(v != property_list.end())
    {
        value = v->second;
        return true;
    }
    return false;

}

