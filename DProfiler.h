#ifndef DPROFILER_H
#define DPROFILER_H
#include <QDebug>
#include <QString>
#include <QMap>
#include <ctime>
#include <sys/time.h>


namespace PROFILER {

    QMap<QString, unsigned> start;
    void START_PBLOCK(const QString &NAME)
    {
        unsigned start_time = clock();
        start[NAME] = start_time;
        qDebug()<<"START BLOCK WITH"<<NAME<<"NAME-----------"<<start_time;
    }
    void END_PBLOCK(const QString &NAME)
    {
        unsigned end_time = clock();
        qDebug()<<"INTERVAL IS"<< end_time - start.value(NAME) - 10<<"ms";
        qDebug()<<"END BLOCK WITH"<<NAME<<"NAME-----------"<<end_time;
    }
//============================================================================================
    typedef struct tp
    {
        struct timeval t;
        QString name;
    }tp;
    QVector<tp> time_points;


    int find_tp(const QString &NAME)
    {
        int n = 0;
        for(auto it: time_points)
        {
            if(it.name == NAME)
                return n;
            n++;
        }
        return -1;
    }
    void time_point(const QString &name)
    {
        struct timeval t;
        gettimeofday(&t, nullptr);
        if(find_tp(name) != -1)
        {
            int n = 2;
            while(find_tp(QString("%1_%2").arg(name).arg(n)) != -1)
                n++;
            time_points.push_back( tp({t, QString("%1_%2").arg(name).arg(n)}) );
        }
        else
            time_points.push_back( tp({t, name}) );
    }
    void time_point(const int &index)
    {
        time_point(QString("%1").arg(index));
    }
    void print_tp_range(const QString &name1, const QString &name2, const QString custom_message = "")
    {
        timeval t1, t2;
        int n1 = find_tp(name1);
        int n2 = find_tp(name2);

        if(n1 == -1 || n2 == -1)
        {
            qDebug()<<"Wrong time point name ("<<name1<<","<<name2<<")";
            return ;
        }

        t1 = time_points[n1].t;
        t2 = time_points[n2].t;

        int dif = (1000000L * (t2.tv_sec - t1.tv_sec)) + (t2.tv_usec - t1.tv_usec);
        int sec = (int)(dif/1000000L);
        int usec = dif - sec*1000000L;

        if(custom_message.size())
            qDebug()<<custom_message<<sec<<"s"<<usec<<"us";
        else
            qDebug()<<"TIME ("<<name1<<"-"<<name2<<"):"<<sec<<"s"<<usec<<"us";
    }
    void print_tp_range(const int &index1, const int &index2, const QString custom_message = "")
    {
        print_tp_range(QString("%1").arg(index1),QString("%1").arg(index2), custom_message);
    }
    void print_all_tp_ranges()
    {
        timeval t1, t2;
        for(int i=0;i!=time_points.size()-1;++i)
        {
            t1 = time_points[i].t;
            t2 = time_points[i+1].t;
            int dif = (1000000L * (t2.tv_sec - t1.tv_sec)) + (t2.tv_usec - t1.tv_usec);
            int sec = (int)(dif/1000000L);
            int usec = dif - sec*1000000L;
            qDebug()<<"Range"<<i<<"("<<time_points[i].name<<"-"<<time_points[i+1].name<<"):"<<sec<<"s"<<usec<<"us";
        }
        print_tp_range(time_points.first().name, time_points.last().name, "Total time");
    }
    void clear_time_points()
    {
        time_points.clear();
    }
}
#endif // DPROFILER_H
