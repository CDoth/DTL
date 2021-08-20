#ifndef DTHREADHANDLER_H
#define DTHREADHANDLER_H

#include "DArray.h"
#include "DHolder.h"
#include "DBox.h"

#include <thread>
void get_thread_info(int size, int threads, int thread_index, int &start_pos, int &task_on_thread);
int get_thread_id();
class DThreadHandler
{
public:
    DThreadHandler(){}
    DThreadHandler(const DThreadHandler&)
    {

    }
    ~DThreadHandler(){}
    struct base_o
    {
        virtual void go() = 0;
        virtual ~base_o(){}
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
            __el_begin = nullptr;
            __el_end = nullptr;
        }
        ~thread_context()
        {
            for(int i=0;i!=end_list.size();++i) delete end_list[i];
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

//-------------------------------------------------------------------------------------------------------------
//    template <class CloseF>
//    void set_close_f(CloseF& cf)
//    {
//        context.get().end_list.push_back(make_holder(cf));
//        context.get().__el_begin = context.get().end_list.begin();
//        context.get().__el_end = context.get().end_list.end();
//    }

//    void set_repeat(int n){if(n<1) n=1; context.get().repeat = n;}
//    void set_start_sleep(long ms) { context.get().sleep_on_start = std::chrono::milliseconds(ms);}
//    void set_midle_sleep(long ms) { context.get().sleep_on_midle = std::chrono::milliseconds(ms);}
//    void set_end_sleep(long ms) { context.get().sleep_on_end = std::chrono::milliseconds(ms);}
//    void set_repeat_sleep(long ms) { context.get().sleep_on_repeat = std::chrono::milliseconds(ms);}
//-------------------------------------------------------------------------------------------------------------
    template <class CloseF>
    void set_close_f(CloseF& cf)
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
//-------------------------------------------------------------------------------------------------------------


    template <class TargetF, class ... Args>
    static void
    fshell(TargetF&& tf,  Args&& ... a)
    {tf(a...);}

    template <class TargetF, class ... Args>
    static void
    shell(DHolder<thread_context> tch, TargetF&& tf,  Args&& ... a)
    {
        const thread_context* tc = tch.constPtr();
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
    static void
    shell2(thread_context &tc, TargetF&& tf,  Args&& ... a)
    {
        std::this_thread::sleep_for(tc.sleep_on_start);
        int c=tc.repeat;
        while(c--) {
            tf(a...);
            if(c) std::this_thread::sleep_for(tc.sleep_on_repeat);
        }
        std::this_thread::sleep_for(tc.sleep_on_midle);
        auto it = tc.__el_begin;
        while(it != tc.__el_end) (*it++)->go();
        std::this_thread::sleep_for(tc.sleep_on_end);
    }



    template <class TargetF, class ... Args>
    void start(TargetF tf, Args... a)
    {
//        _t = std::thread(shell<TargetF, Args...>, context, tf, a...);
//        _t = std::thread(fshell<TargetF, Args...>, tf, a...);
        _t = std::thread(shell2<TargetF, Args...>,std::ref(context), tf, a...);
    }
    inline void join()
    {if(_t.joinable()) _t.join();}
    inline void join_if(bool s)
    {if(_t.joinable() && s) _t.join();}
    inline void detach()
    {if(_t.joinable()) _t.detach();}
    inline void detach_if(bool s)
    {if(s) _t.detach();}

//    void clear_close_f()
//    {context.get().end_list.clear();}
    void clear_close_f()
    {context.end_list.clear();}

public:
    std::thread _t;
//    DHolder<thread_context> context;
    thread_context context;
};

/*
 test block

        DThreadHandler th;
        th.set_close_f(func2);
        th.set_close_f(f);
        th.set_repeat(2);

        th.set_start_sleep(2000);
        th.set_midle_sleep(2000);
        th.set_repeat_sleep(2000);
        th.set_end_sleep(2000);

        th.start(test, 123, 56.78);
        th.join();
 */
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------

class DThreadMaster
{
public:
    explicit DThreadMaster(int max_threads) : thread_box(max_threads) {}
    DThreadHandler* get_thread()
    {

        auto h = thread_box.pull();
        if(h)
        {
            h->clear_close_f();
            h->set_close_f(*h);
        }
        return h;
    }

    void join_all()
    {
//        for(int i=0;i!=thread_box.size();++i)
//            thread_box[i].join();
    }

private:
    struct wrap : public DThreadHandler
    {
        void operator()(){box->push(this);}
    private:
        DBox<wrap>* box;
    };
    DBox<wrap> thread_box;
};
#endif // DTHREADHANDLER_H
