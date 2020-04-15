#include <ui/gtk/bree_gtk_context.h>


namespace bree {
namespace ui{
namespace gtk{
namespace context{
void destroy_gtk_context_callback(bree_object_ptr){

}
bree_context_ptr context_new(){
bree_gtk_context_ptr l_context = std::make_shared<bree_gtk_context>();
bree::context::global::register_context(l_context->cast<bree_context>(),&destroy_gtk_context_callback);
return l_context;
}
}/// context
}/// gtk
}/// ui
}/// bree
