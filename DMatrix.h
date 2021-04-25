#ifndef DMATRIX_H
#define DMATRIX_H
#include <DWatcher.h>
#include "dmem.h"

#include <QDebug>
template <class T>
class DMatrixData
{
public:
    int size;
    int w;
    int h;
    T* data;
    int placed;
    void freeData()
    {
        if(!placed) free_mem(data);
    }
    void clear()
    {
        size = 0;
        w = 0;
        h = 0;
        placed = 0;
        data = nullptr;
    }

    DMatrixData()
    {
//        qDebug()<<"DMatrixData: create empty"<<this;
        clear();
    }
    DMatrixData(int _w, int _h, T* to = nullptr)
    {
//        qDebug()<<"DMatrixData: create with size:"<<_w<<_h<<this;
        clear();
        w = _w;
        h = _h;
        size = w*h;
        data = to? placed = 1, to: get_mem<T>(size);
        zero_mem(data, size);
    }
    DMatrixData(const DMatrixData& r, T* to = nullptr)
    {
//        qDebug()<<"DMatrixData: copy from:"<<&r<<"to:"<<this<<"place:"<<to;
        clear();
        w = r.w;
        h = r.h;
        size = r.size;
        freeData();
        data = to? placed = 1, to: get_mem<T>(size);
        copy_mem(data, r.data, size);
    }
    ~DMatrixData()
    {
//        qDebug()<<"DMatrixData: destroy"<<this;
        freeData();
        clear();
    }
};
template <class T>
class DMatrix
{
public:
    DMatrix();
    DMatrix(int w, int h, T *place = nullptr);

    //-----------------------------------
    DMatrix(const DMatrix &m, T*p = nullptr);
    DMatrix& operator=(const DMatrix&);
    //-----------------------------------
    ~DMatrix()
    {
        if(w->pull() == 0) {delete data();delete w;}
    }

    typedef T* iterator;
    typedef const T* const_iterator;

    iterator begin();
    iterator end();
    T* operator[](int i);

    const_iterator constBegin() const;
    const_iterator constEnd() const;
    const T* operator[](int i) const;

    void fill(const T&);
    void run(void (*F)(T&));
    void run_to(const T& (*F)(const T&), DMatrix* to);
    void copy(const DMatrix &from);
    void unite(const DMatrix<T> &with);
    void multiply(const DMatrix<T> &with);
    void multiply(const T &value);

    int height() const {return data()->h;}
    int width()  const {return data()->w;}
    int area()   const {return data()->size;}

    void swap(DMatrix<T>&);
    inline void make_unique()
    {
        w->refDown();
        w = new DDualWatcher(clone(), CloneWatcher);
    }
    void setMode(WatcherMode m)
    {
        if(m != w->mode() && !(data()->placed && w->is_share()))
        {
            w->refDown();
            w = w->otherSide();
            if(w->otherSideRefs() == 0)
                w->disconnect();
            w->refUp();
        }
    }
public:
    DMatrixData<T>* clone() {return new DMatrixData<T>(*data());}
    void detach()
    {
        if(!w->is_unique())
        {
            if(w->is_share() && w->otherSideRefs())
            {
                if(data()->placed)
                    w->otherSide()->disconnect(clone());
                else
                    w->disconnect(clone());
            }
            else if(w->is_clone())
            {
                w->refDown();
                w = new DDualWatcher(clone(), CloneWatcher);
            }
        }
    }
    void detach_debug()
    {
        qDebug()<<"DMatrix: detach_debug()";
        if(!w->is_unique())
        {
            qDebug()<<"---1";
            if(w->is_share() && w->otherSideRefs())
            {
                qDebug()<<"---2";
                if(data()->placed)
                {
                    w->otherSide()->disconnect(clone());
                    qDebug()<<"---3";
                }
                else
                {
                    w->disconnect(clone());
                    qDebug()<<"---4";
                }
            }
            else if(w->is_clone())
            {
                qDebug()<<"---5"<<w->refs();
                w->refDown();
                w = new DDualWatcher(clone(), CloneWatcher);
            }
        }
    }
    DDualWatcher* w;
    DMatrixData<T>* data()
    {return reinterpret_cast<DMatrixData<T>* >(w->d());}
    const DMatrixData<T>* data() const
    {return reinterpret_cast<const DMatrixData<T>* >(w->d());}


    void _set_mode(WatcherMode m)
    {
        if(m != w->mode())
        {
            w->refDown();
            w = w->otherSide();
            if(w->otherSideRefs() == 0)
                w->disconnect();
            w->refUp();
        }
    }
};
template <class T>
DMatrix<T>::DMatrix()
{
    DMatrixData<T>* d = new DMatrixData<T>();
    w = new DDualWatcher(d, CloneWatcher);
}
template <class T>
DMatrix<T>::DMatrix(int width, int height, T* place)
{
    DMatrixData<T>* d = new DMatrixData<T>(width,height,place);
    auto mode = place ? ShareWatcher : CloneWatcher;
    w = new DDualWatcher(d, mode);
}
template <class T>
DMatrix<T>::DMatrix(const DMatrix<T> &m, T* place)
{
    if(place)
    {
        DMatrixData<T>* d = new DMatrixData<T>(*m.data(),place);
        w = new DDualWatcher(d, CloneWatcher);
    }
    else
    {
        w = m.w;
        w->refUp();
        _set_mode(CloneWatcher);
    }
}
template <class T>
void DMatrix<T>::swap(DMatrix<T>& with)
{
    std::swap(w, with.w);
}
template <class T>
DMatrix<T>& DMatrix<T>::operator=(const DMatrix<T>& m)
{
    if(w != m.w)
    {
        if(data()->placed) copy(m);
        else
        {
            DMatrix<T> tmp(m);
            tmp.swap(*this);
        }
    }
    return *this;
}
template <class T>
void DMatrix<T>::fill(const T& v)
{
    iterator b = begin();
    iterator e = end();
    while(b != e) *b++ = v;
}
template <class T>
void DMatrix<T>::run(void (*F)(T&))
{
    iterator b = begin();
    iterator e = end();
    while(b != e) F(*b++);
}
template <class T>
void DMatrix<T>::run_to(const T& (*F)(const T&), DMatrix* to)
{
    iterator it = begin();
    iterator it_to = to->begin();
    while(it != end()) *it_to++ = F(*it++);
}
//---------------------------------------------------------------------------
template <class T>
typename DMatrix<T>::iterator DMatrix<T>::begin()
{
    detach();
    return data()->data;
}
template <class T>
typename DMatrix<T>::iterator DMatrix<T>::end()
{
    detach();
    return data()->data + data()->size;
}
template <class T>
T* DMatrix<T>::operator[](int i)
{
    detach();
    return &data()->data[i*data()->h];
}
template <class T>
typename DMatrix<T>::const_iterator DMatrix<T>::constBegin() const
{
    return data()->data;
}
template <class T>
typename DMatrix<T>::const_iterator DMatrix<T>::constEnd() const
{
    return data()->data + data()->size;
}
template <class T>
const T* DMatrix<T>::operator[](int i) const
{
    return &data()->data[i*data()->h];
}
//---------------------------------------------------------------------------
template <class T>
void DMatrix<T>::unite(const DMatrix<T>& with)
{
    iterator b = begin();
    iterator e = end();
    const_iterator b2 = with.constBegin();
    while(b!=e) *b++ += *b2++;
}
template <class T>
void DMatrix<T>::multiply(const DMatrix<T>& with)
{
    iterator b = begin();
    iterator e = end();
    const_iterator b2 = with.constBegin();
    while(b!=e) *b++ *= *b2++;
}
template <class T>
void DMatrix<T>::multiply(const T &value)
{
    iterator b = begin();
    iterator e = end();
    while(b!=e) *b++ *= value;
}
template <class T>
void DMatrix<T>::copy(const DMatrix& from)
{
    detach();
    copy_mem(data()->data, from.data()->data, data()->size);
}


#endif // DMATRIX_H
