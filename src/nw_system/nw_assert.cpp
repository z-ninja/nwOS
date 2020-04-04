#include <nw_system/nw_assert.h>
#include <condition_variable>
#include <boost/bind.hpp>
#include <functional>
#ifndef NW_SYSTEM_NO_GTK
#include <gtk/gtk.h>
#endif
#include <mutex>
#include <queue>
#include <nw_system/export.h>
#include <nw_system/base.h>
#include <nw_system/nw_object.h>
#include <nw_system/nw_event.h>
#include <nw_system/nw_main.h>
#include <nw_system/nw_service.h>
#include <cassert>
#include <iostream>
#include <sstream>
#include <exception>
#include <mutex>
std::thread::id MAIN_THREAD_ID = std::this_thread::get_id();
std::mutex main_thread_mutex;
NW_PUBLIC std::thread::id nw_get_main_thread_id(){
std::lock_guard<std::mutex> g(main_thread_mutex);
return MAIN_THREAD_ID;
}
void nw_set_thread_id(std::thread::id id){
std::lock_guard<std::mutex> g(main_thread_mutex);
MAIN_THREAD_ID = id;
}
std::mutex assert_mutex;
NW_PUBLIC void nw_assertion_faild(bool b,std::string expresion,std::string msg,std::string file_name,int line_number)
{
    if(!b)
    {
        std::stringstream ss;
        #ifdef NW_WINDOWS
        #ifdef NW_ARCH_64
        ss << "windows-64 ";
        #else
        ss << "windows-32 ";
        #endif
        #elif defined NW_LINUX
        #ifdef NW_ARCH_64
        ss << "linux-64 ";
        #else
        ss << "linux-32 ";
        #endif
        #elif defined NW_MAC_OS_X
        #ifdef NW_ARCH_64
        ss << "macosx-64 ";
        #else
        ss << "macosx-32 ";
        #endif
        #endif // NW_WINDOWS
        //ss << NW_X_SERVER_VERSION << std::endl;
        ss << "NW ASSERTION FAILD: " << expresion << std::endl;
        ss << "MAIN THREAD ID: " << nw_get_main_thread_id() << std::endl;
        ss << "THREAD ID: " << std::this_thread::get_id() << std::endl;
        ss << "FILE: " << file_name << ":" << line_number << std::endl;
        if(msg.length())
            ss << "MSG: " << msg << std::endl;


        time_t tmt;
        time(&tmt);
        std::string log_str = ctime(&tmt);
        log_str = "\n"+log_str;
        //assert_mutex.lock();
        std::cout << ss.str();
        //file_append_content("etc/logs/asserts.log",log_str+ss.str());
        //assert_mutex.unlock();

    FILE * pFile;
    pFile = fopen ( "./etc/logs/asserts.txt" , "ab" );
    if (pFile==NULL)
    {
        std::cerr << "assert file write error: " << std::endl;
        return;
    }

    /*size_t writen =*/ fwrite (ss.str().c_str() , sizeof(char), ss.str().length(), pFile);
    fclose (pFile);

        //if(NW_X_SERVER_VERSION > 200003)
        //nw_user_report_assertion_failed(ss.str());
     //nw_system_service_quit(12);
      exit(12);
      //std::terminate();
    }


}

