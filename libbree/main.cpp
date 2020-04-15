#include <base/bree.h>
#include <iostream>
#include <thread>
#include <ui/gtk/bree_gtk_context.h>
#include <net/bree_boost_asio_context_thread.h>
#include <unistd.h>
void thread_func_test()
{
    bree_context_ptr l_thread = bree::context::context_thread_new();

    bree::timer::interval(l_thread,[&,l_thread]()->bool
    {
        //std::cout << "thread bre" << std::endl;
        bree::timer::timeout(l_thread,[&]()->void{
            //      std::cout << "sleep" << std::endl;
            /// sleep(1);
        },2000);
        return false;
    },199);
}
void testic(bree_context_ptr a_context)
{
    bree_object_ptr l_object =  a_context->cast<bree_object>();
}
int main(int argc,char*argv[])
{
    int max_threads = 10;
    gtk_init(&argc,&argv);

    bree_context_ptr l_context = bree::ui::gtk::context::context_new();
    bree::bree_init(argc,argv, l_context);
    std::thread threads[10];
    for(int i=0; i<max_threads; i++)
    {
        threads[i] = std::thread(thread_func_test);
    }
    bree_context_ptr l_asio = bree::net::context::context_new();

    bree::timer::interval(l_asio,[&]()->bool{
                   //       std::cout << "asio" << std::endl;
                          return false;
    },20);

    GtkWidget*win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request(win,300,300);
    GtkWidget*button = gtk_button_new_with_label("label of button");
    gtk_container_add(GTK_CONTAINER(win),button);

    gtk_widget_show_all(win);

    bree::main_thread::interval([&]()->bool
    {
        bree::main_thread::timeout([&]()->void
        {
           /// std::cout << "aka" << std::endl;
        },1000);

        return false;
    },100);

    bree::main_thread::timeout([&]()->void
    {

        std::cout << "COOL QUIT!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
        bree::bree_exit(20);
        std::cout << "cull out done" << std::endl;
    },5000);

    bree::bree_run();

    for(int i=0; i<max_threads; i++)
    {
        threads[i].join();
    }
    std::cout << "run finished: " << bree::bree_exit_code() << std::endl;

    //sleep(5);
    return 0;

}
