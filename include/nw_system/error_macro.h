#ifndef NW_ERROR_MACRO_H_INCLUDED
#define NW_ERROR_MACRO_H_INCLUDED
// custom error_category example
#include <system_error>   // std::is_error_condition_enum, std::error_category,
// std::error_condition, std::error_code
#include <string>
#include <iostream>
#define CATEGORY_CODE(category_name) nw::error::ns_##category_name::code
#define CATEGORY_NAMESPACE_BEGIN(category_name)\
namespace nw\
{\
namespace error\
{\
namespace ns_##category_name{

#define CATEGORY_NAMESPACE_END }}}

#define DEFINE_CATEGORY(category_name,...)\
namespace nw\
{\
namespace error\
{\
namespace ns_##category_name{\
enum code {\
no_error = 0,\
__VA_ARGS__\
};\
}\
}\
\
}\
namespace std\
{\
template<> struct is_error_condition_enum<nw::error::ns_##category_name::code> : public true_type {};\
}\
namespace nw\
{\
namespace error\
{\
namespace ns_##category_name{\
namespace detail\
{\
class category : public std::error_category{\
public:\
    virtual const char* name() const noexcept\
    {\
        return #category_name;\
    }\
virtual std::error_condition default_error_condition (int ev) const noexcept override final;\
virtual bool equivalent (const std::error_code& code, int condition) const noexcept override final\
    {\
        return *this==code.category() &&\
               static_cast<int>(default_error_condition(code.value()).value())==condition;\
    }\
    virtual std::string message(int ev) const noexcept override final;\
};\
}\
extern inline const nw::error::ns_##category_name::detail::category &category_##category_name()\
{\
    static nw::error::ns_##category_name::detail::category c;\
    return c;\
}\
\
inline std::error_code make_error_code(code e)\
{\
    return {static_cast<int>(e), category_##category_name()};\
}\
inline std::error_condition make_error_condition (code e)\
{\
    return std::error_condition(static_cast<int>(e), category_##category_name());\
}\
}\
inline std::error_code make_error_code(nw::error::ns_##category_name::code e)\
{\
    return nw::error::ns_##category_name::make_error_code(e);\
}\
}\
}




#endif
