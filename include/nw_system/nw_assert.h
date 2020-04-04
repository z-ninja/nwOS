#ifndef NW_ASSERT_H
#define NW_ASSERT_H
#include <string>
#include <thread>
std::thread::id nw_get_main_thread_id();
void nw_assertion_faild(bool b,std::string expresion,std::string msg,std::string file_name,int line_number);
#define NW_ASSERT(x,msg)  nw_assertion_faild(x,#x,msg,__FILE__,__LINE__)

#endif // NW_ASSERT_H_INCLUDED
