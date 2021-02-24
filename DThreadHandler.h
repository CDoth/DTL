#ifndef DTHREADHANDLER_H
#define DTHREADHANDLER_H

#include "DArray.h"
#include "DBox.h"

#include <thread>

class DThreadHandler
{
public:
    DThreadHandler(){}
    ~DThreadHandler(){}
    struct base_o
    {
        virtual void go() = 0;
        virtual ~base_o(){};
    };
    template <class T>
    struct holder : public base_o
    {
        holder(T& p) : o(&p){}
        void go(){(*o)();}
        ~holder(){}
    private:
        T* o;
    };
    template <class T>
    static holder<T>* make_holder(T& p){return new holder<T>(p);}

    struct thread_context
    {
    public:
        typedef std::chrono::milliseconds ms_t;
        thread_context()
        {
            repeat = 1;
            sleep_on_start = std::chrono::milliseconds(0);
            sleep_on_midle = std::chrono::milliseconds(0);
            sleep_on_end = std::chrono::milliseconds(0);
            sleep_on_repeat = std::chrono::milliseconds(0);
        }
        ~thread_context()
        {
            auto it = __el_begin;
            while(it!=__el_end) delete *it++;
        }
        friend class DThreadHandler;

    private:
        int repeat;

        ms_t sleep_on_start;
        ms_t sleep_on_midle;
        ms_t sleep_on_end;
        ms_t sleep_on_repeat;

        DArray<base_o*> end_list;
        DArray<base_o*>::iterator __el_begin;
        DArray<base_o*>::iterator __el_end;
    };
    template <class CloseF>
    void set_close_f(CloseF cf)
    {
        context.end_list.push_back(make_holder(cf));
        context.__el_begin = context.end_list.begin();
        context.__el_end = context.end_list.end();
    }

    void set_repeat(int n){if(n<1) n=1; context.repeat = n;}
    void set_start_sleep(long ms) { context.sleep_on_start = std::chrono::milliseconds(ms);}
    void set_midle_sleep(long ms) { context.sleep_on_midle = std::chrono::milliseconds(ms);}
    void set_end_sleep(long ms) { context.sleep_on_end = std::chrono::milliseconds(ms);}
    void set_repeat_sleep(long ms) { context.sleep_on_repeat = std::chrono::milliseconds(ms);}

    template <class TargetF, class ... Args>
    static void
    fshell(TargetF&& tf,  Args&& ... a)
    {tf(a...);}

    template <class TargetF, class ... Args>
    static void
    shell(thread_context* tc, TargetF&& tf,  Args&& ... a)
    {
        std::this_thread::sleep_for(tc->sleep_on_start);
        int c=tc->repeat;
        while(c--) {
            tf(a...);
            if(c) std::this_thread::sleep_for(tc->sleep_on_repeat);
        }
        std::this_thread::sleep_for(tc->sleep_on_midle);
        auto it = tc->__el_begin;
        while(it != tc->__el_end) (*it++)->go();
        std::this_thread::sleep_for(tc->sleep_on_end);
    }



    template <class TargetF, class ... Args>
    void start(TargetF tf, Args... a)
    {
        join_if();
        _t = std::thread(shell<TargetF, Args...>, &context, tf, a...);
    }
    void join_if()
    {if(_t.joinable()) _t.join();}
    void clear_close_f()
    {context.end_list.clear();}

private:
    std::thread _t;
    thread_context context;
};

//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------

class DThreadMaster
{
public:
    explicit DThreadMaster(int max_threads) : thread_box(max_threads) {}
    DThreadHandler* get_thread()
    {
        DBox<DThreadHandler>::item_holder ih = thread_box.pull();
        if(ih)
        {
            ih->clear_close_f();
            ih->set_close_f(*ih);
        }
        return ih;
    }

    void join_all()
    {
//        for(int i=0;i!=thread_box.size();++i)
//            thread_box[i].join_if();
    }

private:
    DBox<DThreadHandler> thread_box;
};
#endif // DTHREADHANDLER_H
