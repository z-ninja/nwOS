#ifndef BREE_OBJECT_H
#define BREE_OBJECT_H
#include <condition_variable>
#include <functional>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <map>

#include <base/bree_export.h>
#include <base/bree_event.h>

template <typename bree_base>
inline std::shared_ptr<bree_base>
shared_from_base(std::enable_shared_from_this<bree_base>* base)
{
    return base->shared_from_this();
}
template <typename bree_base>
inline std::shared_ptr<const bree_base>
shared_from_base(std::enable_shared_from_this<bree_base> const* base)
{
    return base->shared_from_this();
}
template <typename That>
inline std::shared_ptr<That>
shared_from(That* that)
{
    return std::static_pointer_cast<That>(shared_from_base(that));
}
class BREE_PUBLIC bree_object_base : public std::enable_shared_from_this<bree_object_base> {

};
#define BREE_OBJ_DEF( name) name##_ptr shared_from_this(){\
return shared_from(this);\
}\
virtual bree_object_ptr to_object(){\
return shared_from_this();\
}\
virtual std::string get_type_name(){\
return #name;\
}

namespace bree{
class bree_context_manager;
}
class bree_object;
typedef std::shared_ptr<bree_object> bree_object_ptr;
class bree_context;
typedef std::shared_ptr<bree_context> bree_context_ptr;

class BREE_PUBLIC bree_object : public bree_object_base
{
    friend class bree::bree_context_manager;
protected:
    int m_flags;
public:
    BREE_OBJ_DEF(bree_object);
    bree_object();
    virtual~bree_object();
    virtual void exit(bree_context_ptr);
    virtual bool on_event(bree_event_ptr);
    template<class T> std::shared_ptr<T> cast()
    {
        return shared_from((T*)this);
    }
    template<class T> T* cast_native()
    {
        return dynamic_cast<T*>(this);
    }
};




#endif // BREE_OBJECT_H
