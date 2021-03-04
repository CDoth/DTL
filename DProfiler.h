#ifndef DPROFILER_H
#define DPROFILER_H
#include <QDebug>
#include <QString>
#include <QMap>
#include <ctime>
#include <sys/time.h>
#include <iostream>

namespace PROFILER {

    static std::map<std::string, unsigned> start;
    static void START_PBLOCK(const std::string &NAME)
    {
        unsigned start_time = clock();
        start[NAME] = start_time;
        std::cout << "---------- START BLOCK " << NAME.c_str() << "on:" << start_time <<std::endl;
//        qDebug()<<"---------- START BLOCK "<<NAME.c_str()<<"on:"<<start_time;
    }
    static void END_PBLOCK(const std::string &NAME)
    {
        unsigned end_time = clock();
        std::cout << "---------- END BLOCK" << NAME.c_str() << "on:" << end_time << "INTERVAL:" << end_time - start[NAME] <<std::endl;
//        qDebug()<<"---------- END BLOCK"<<NAME.c_str()<<"on:"<<end_time<<"INTERVAL:"<<end_time - start[NAME];
    }
//============================================================================================
    typedef struct tp
    {
        struct timeval t;
        int n_name;
        std::string s_name;
    }tp;
    static std::vector<tp> time_points;

    int sec_dif(timeval* t1, timeval* t2)
    {
        int dif = (1000000L * (t2->tv_sec - t1->tv_sec)) + (t2->tv_usec - t1->tv_usec);
        return (int)(dif/1000000L);
    }
    int usec_dif(timeval* t1, timeval* t2)
    {
        return (1000000L * (t2->tv_sec - t1->tv_sec)) + (t2->tv_usec - t1->tv_usec);
    }
    timeval time_dif(timeval* t1, timeval* t2)
    {
        int dif = (1000000L * (t2->tv_sec - t1->tv_sec)) + (t2->tv_usec - t1->tv_usec);
        int sec = (int)(dif/1000000L);
        int usec = dif - sec*1000000L;
        return {sec, usec};
    }
    static int find_tp(int name)
    {
        int n = 0;
        for(auto it: time_points)
        {
            if(it.n_name == name)
                return n;
            ++n;
        }
        return -1;
    }
    static int find_tp(const char* name)
    {
        int n = 0;
        for(auto it: time_points)
        {
            if(strcmp( it.s_name.c_str(), name) == 0)
                return n;
            ++n;
        }
        return -1;
    }
    static void time_point(const char* name)
    {
        struct timeval t;
        gettimeofday(&t, nullptr);
        if(int n = find_tp(name) < 0) time_points.push_back( tp{t, 0, name} );
        else time_points[n].t = t;
    }
    static void time_point(int name)
    {
        struct timeval t;
        gettimeofday(&t, nullptr);
        if(int n = find_tp(name) < 0) time_points.push_back( tp{t, name, std::to_string(name)} );
        else time_points[n].t = t;
    }
    template <class name_t1, class name_t2>
    void print_tp_range(name_t1 name1, name_t2 name2, const char* custom_message = nullptr)
    {
        timeval t1, t2;
        int n1 = find_tp(name1);
        int n2 = find_tp(name2);

        if(n1 == -1 || n2 == -1)
        {
            std::cout << name1 <<"," << name2 <<")"<<std::endl;
//            qDebug()<<"Wrong time point name ("<<name1<<","<<name2<<")";
            return ;
        }

        t1 = time_points[n1].t;
        t2 = time_points[n2].t;

        int dif = (1000000L * (t2.tv_sec - t1.tv_sec)) + (t2.tv_usec - t1.tv_usec);
        int sec = (int)(dif/1000000L);
        int usec = dif - sec*1000000L;

        if(custom_message)
            std::cout << custom_message << " " << sec << " s " << usec << " us" << std::endl;
//            qDebug()<<custom_message<<sec<<"s"<<usec<<"us";
        else
            std::cout << "TIME (" << name1 << "-" << name2 << "):" << sec << "s" << usec << "us" <<std::endl;
//            qDebug()<<"TIME ("<<name1<<"-"<<name2<<"):"<<sec<<"s"<<usec<<"us";
    }
    static void print_all_tp_ranges()
    {
        tp* t1 = nullptr;
        tp* t2 = nullptr;
        for(auto it: time_points)
        {
            t2 = &it;
            if(t1) print_tp_range(t1->s_name.c_str(), t2->s_name.c_str());
            t1 = &it;
        }
    }
    static void clear_time_points()
    {
        time_points.clear();
    }
}
#endif // DPROFILER_H
