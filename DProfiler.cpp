#include "DProfiler.h"

namespace PROFILER {

std::vector<tp> time_points = std::vector<tp>();

double fsec_dif(timeval* t1, timeval* t2)
{
    double dif = (1000000L * (t2->tv_sec - t1->tv_sec)) + (t2->tv_usec - t1->tv_usec);
    return (dif/1000000.0);
}
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
timeval gettime()
{
    struct timeval time;
    gettimeofday(&time, nullptr);
    return time;
}
int find_tp(int name)
{
//    qDebug()<<"::::: CALL: find_tp:"<<"name:"<<name;
    int n = 0;
    for(auto it: time_points)
    {
//        qDebug()<<"find_tp: name:"<<name<<"current:"<<it.n_name;
        if(it.n_name == name)
        {
//            qDebug()<<"----find_tp: FIND: name:"<<it.n_name<<"time:"<<it.t.tv_sec<<it.t.tv_usec<<"index:"<<n;
            return n;
        }
        ++n;
    }
    return -1;
}
int find_tp(const char* name)
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
void time_point(const char* name)
{
    struct timeval t;
    gettimeofday(&t, nullptr);
    if(int n = find_tp(name) < 0) time_points.push_back( tp{t, 0, name} );
    else time_points[n].t = t;
}
void time_point(int name)
{
    struct timeval t;
    gettimeofday(&t, nullptr);

//    qDebug()<<"::::: CALL: time_point:"<<"name:"<<name<<"time:"<<t.tv_sec<<t.tv_usec;

    int n = find_tp(name);
    if(n < 0)
    {
//        qDebug()<<"time_point: add new point:"<<name<<"time:"<<t.tv_sec<<t.tv_usec;
        time_points.push_back( tp{t, name, std::to_string(name)} );
    }
    else
    {
//        qDebug()<<"time_point: replace point:"<<name<<"find index:"<<n<<"time: from:"<<time_points[n].t.tv_sec<<time_points[n].t.tv_usec<<"to:"<<t.tv_sec<<t.tv_usec;
        time_points[n].t = t;
    }
}
int timeval2usec(timeval *t) {
    return (1000000L * (t->tv_sec)) + (t->tv_usec);
}

void waitUsec(int usec) {
    struct timeval s, e;
    gettimeofday(&s, NULL);

    for(;;) {
        gettimeofday(&e, NULL);
        if(usec_dif(&s, &e) > usec) {
            break;
        }
    }

}

}
