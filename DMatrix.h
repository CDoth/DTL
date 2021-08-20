#ifndef DMATRIX_H
#define DMATRIX_H
#include <DWatcher.h>
#include "dmem.h"

#include <QDebug>
enum DMatrixOrientation{DMATRIX_HORIZONTAL_ORIENTATION, DMATRIX_VERTICAL_ORIENTATION};
template <class T>
class DMatrixData
{
public:
    int size;
    int width;
    int height;
    int linesize;
    T* data;
    int placed;
    DMatrixOrientation orientation;
    void freeData()
    {
        if(!placed) free_mem(data);
    }
    void clear()
    {
        size = 0;
        width = 0;
        height = 0;
        placed = 0;
        data = nullptr;
    }

    DMatrixData()
    {
        clear();
    }
    DMatrixData(int _w, int _h, T* to = nullptr)
    {
        clear();
        width = _w;
        height = _h;
        size = width*height;
        linesize = width;
        orientation = DMATRIX_HORIZONTAL_ORIENTATION;
        data = to? placed = 1, to: get_mem<T>(size);
        zero_mem(data, size);
    }
    DMatrixData(const DMatrixData& r, T* to = nullptr)
    {
        clear();
        width = r.width;
        height = r.height;
        linesize = r.linesize;
        orientation = r.orientation;
        size = r.size;
        freeData();
        data = to? placed = 1, to: get_mem<T>(size);
        copy_mem(data, r.data, size);
    }


    ~DMatrixData()
    {
//        freeData();
//        clear();
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
//        if(w->pull() == 0) {delete data();delete w;}
    }

    typedef T* iterator;
    typedef const T* const_iterator;

    iterator begin();
    iterator end();
    T* operator[](int i);
    bool operator==(const DMatrix<T> &m) const;

    const_iterator constBegin() const;
    const_iterator constEnd() const;
    const T* operator[](int i) const;

    void fill(const T&);
    void run(void (*F)(T&));
    void run_to(T (*F)(T), DMatrix<T> *to);
    void copy(const DMatrix &from);
    void unite(const DMatrix<T> &with);
    void multiply(const DMatrix<T> &with);
    void multiply(const T &value);

    void setHorizontalOrientation() {data()->linesize = data()->width; data()->orientation = DMATRIX_HORIZONTAL_ORIENTATION;}
    void setVerticalOrientation() {data()->linesize = data()->height; data()->orientation = DMATRIX_VERTICAL_ORIENTATION;}
    void setOrientation(DMatrixOrientation o)
    {
        switch (o)
        {
            case DMATRIX_HORIZONTAL_ORIENTATION: setHorizontalOrientation(); break;
            case DMATRIX_VERTICAL_ORIENTATION: setHorizontalOrientation(); break;
        }
    }
    DMatrixOrientation getOrientation() const {return data()->orientation;}
    bool isHorizontal() const {return data()->orientation == DMATRIX_HORIZONTAL_ORIENTATION;}
    bool isVertical() const {return data()->orientation == DMATRIX_VERTICAL_ORIENTATION;}

    int height() const {return data()->height;}
    int width()  const {return data()->width;}
    int area()   const {return data()->size;}
    T& value(int x, int y) const {if(isVertical()) std::swap(x,y);  return *(data()->data + (y*data()->linesize + x)); }
    int value_ho(int x, int y) const {return *(data()->data + (y*data()->linesize + x));}
    int value_vo(int x, int y) const {return *(data()->data + (x*data()->linesize + y));}

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
        if(!w->is_unique())
        {
            if(w->is_share() && w->otherSideRefs())
            {
                if(data()->placed)
                {
                    w->otherSide()->disconnect(clone());
                }
                else
                {
                    w->disconnect(clone());
                }
            }
            else if(w->is_clone())
            {
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
    while(b != e)
    {
        F(*b++);
//        F(*b);
//        qDebug()<<"run:"<<*b;
//        ++b;
    }
}
template <class T>
void DMatrix<T>::run_to(T (*F)(T), DMatrix<T> *to)
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
    return data()->data + (i*data()->linesize);
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
    return data()->data + (i*data()->linesize);
}
template <class T>
bool DMatrix<T>::operator==(const DMatrix<T> &m) const
{
    if(m.width() == width() && m.height() == height())
    {
        auto b = m.constBegin();
        auto _b = constBegin();
        auto _e = constEnd();
        int i=0;
        while(_b != _e)
        {
            if(*_b != *b)
            {
                printf("DMatrix: operator==(): value diff: (%d : %d) [%d]\n", *_b, *b, i);
                return false;
            }
            ++_b; ++b; ++i;
        }

//        printf("DMatrix: operator==(): equal\n");
        return true;
    }
    else
    {
        printf("DMatrix: operator==(): size diff\n");
        return false;
    }
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

static void print_matrix(const DMatrix<int> &m, const char *message = nullptr)
{
    if(message) printf("%s:\n", message);
    for(int i=0;i!=m.height();++i)
    {
        for(int j=0;j!=m.width();++j)
        {
            std::cout << m.value(j,i) << ' ';
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}
static void print_matrix(const DMatrix<float> &m, const char *message = nullptr)
{
    if(message) printf("%s:\n", message);
    for(int i=0;i!=m.height();++i)
    {
        for(int j=0;j!=m.width();++j)
            std::cout << m.value(j,i) << ' ';
        std::cout << std::endl;
    }
    std::cout << std::endl;
}
template <class T>
void print_matrix(const DMatrix<T> &m, const char *message = nullptr)
{
    if(message) printf("%s:\n", message);
    for(int i=0;i!=m.height();++i)
    {
        for(int j=0;j!=m.width();++j)
            std::cout << m.value(j,i) << ' ';
        std::cout << std::endl;
    }
    std::cout << std::endl;
}


#endif // DMATRIX_H
