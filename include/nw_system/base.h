#ifndef NW_BASE_H
#define NW_BASE_H
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <nw_system/export.h>
#include <nw_system/nw_macros.h>
#ifdef NW_WINDOWS
#define CONSOLE_SYSTEM_LOG "NW-SYSTEM**LOG**: "
#define CONSOLE_SYSTEM_INFO "NW-SYSTEM**INFO**: "
#define CONSOLE_SYSTEM_WARN "NW-SYSTEM**WARNING**: "
#define CONSOLE_SYSTEM_ERROR "NW-SYSTEM**ERROR**: "
#define CONSOLE_SYSTEM_DEBUG "NW-SYSTEM**DEBUG**: "

#define CONSOLE_COLOR_GREEN(x) x
#define CONSOLE_COLOR_PURPLE(x) x
#else
#define CONSOLE_SYSTEM_LOG "\033[1;36mNW-SYSTEM**LOG**\033[0;0m: "
#define CONSOLE_SYSTEM_INFO "\033[1;34mNW-SYSTEM**INFO**\033[0;0m: "
#define CONSOLE_SYSTEM_WARN "\033[1;35mNW-SYSTEM**WARNING**\033[0;0m: "
#define CONSOLE_SYSTEM_ERROR "\033[1;31mNW-SYSTEM**ERROR**\033[0;0m: "
#define CONSOLE_SYSTEM_DEBUG "\033[1;32mNW-SYSTEM**DEBUG**\033[0;0m: "

#define CONSOLE_COLOR_GREEN(x) "\033[0;32m"+x+"\033[0;0m"
#define CONSOLE_COLOR_PURPLE(x) "\033[0;35m"+x+"\033[0;0m"
#endif
template <typename Base>
inline boost::shared_ptr<Base>
shared_from_base(boost::enable_shared_from_this<Base>* base)
{
    return base->shared_from_this();
}
template <typename Base>
inline boost::shared_ptr<const Base>
shared_from_base(boost::enable_shared_from_this<Base> const* base)
{
    return base->shared_from_this();
}
template <typename That>
inline boost::shared_ptr<That>
shared_from(That* that)
{
    return boost::static_pointer_cast<That>(shared_from_base(that));
}
struct NW_PUBLIC nw_object_base : public boost::enable_shared_from_this<nw_object_base> {};










#endif
