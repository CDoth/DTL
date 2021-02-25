#ifndef DHOLDER_H
#define DHOLDER_H
#include <mutex>
#include <QDebug>
#include "dmem.h"


enum _dholder_modes { ShareNowMode = 0b00000101, ShareMode = 0b00000001, CloneMode = 0b00000010, ConstMode = 0b10000000, ConstShareMode = 0b10000001, ConstCloneMode = 0b10000010};
static bool check_mode(uint8_t m1, uint8_t m2) {return m1 & m2;}

class DHolderStandartModule
{
public:
    struct watcher
    {
        int n;
        void* data;
    };
    watcher* w;

    int add_copy()
    {
        return ++w->n;
    }
    void* pull()
    {
        void* d = w->data;
        if( -- w->n == 0) {delete w; return d;}
        return nullptr;
    }
    void hold_it(void* _data)
    {
        w = new watcher;
        w->n = 1;
        w->data = _data;
    }
    void* detach(void* _data)
    {
        pull();
        hold_it(_data);
        return w->data;
    }
    bool is_unique() const
    {
        return w->n == 1;
    }
    bool setAccessMode(_dholder_modes)
    {
        return false;
    }
    bool isDetachable()
    {
        return !is_unique();
    }
    void* get()
    {
        return w->data;
    }
    const void* get() const
    {
        return w->data;
    }
    uint8_t accessMode() const {return ConstCloneMode;}
};

template <class T, class M = DHolderStandartModule>
class DHolder
{
public:
    DHolder();
    ~DHolder();


    template <class ... Args>
    DHolder(const Args& ... a);
    //-----------------------------
    DHolder(const DHolder& a);
    DHolder(DHolder& a);
    DHolder(const DHolder&& a) = delete;
    DHolder(DHolder&& a) = delete;

    DHolder& operator=(const DHolder& a);
    DHolder& operator=(DHolder& a);
    DHolder& operator=(const DHolder&& a) = delete;
    DHolder& operator=(DHolder&& a) = delete;
    //-----------------------------
    bool is_unique() const;
    T* make_unique();
    T* try_detach();
    void setAccessMode(_dholder_modes);

    T* get() { return reinterpret_cast<T*>( k.get() );}
    const T* get() const { return reinterpret_cast<const T*>( k.get() );}

private:
    inline T* clone()
    { return new T (*reinterpret_cast<T*>(k.get()) ); }
    typedef M kernel;
    kernel k;


    void pull() {if (T* d = reinterpret_cast<T*>( k.pull() ) ) delete d; }
};

template <class T, class M>
DHolder<T,M>::DHolder()
{
    k.hold_it(new T);
    k.setAccessMode(CloneMode);
}
template <class T, class M>
template <class ... Args>
DHolder<T,M>::DHolder(const Args& ... a)
{
    k.hold_it(new T(a...));
    k.setAccessMode(CloneMode);
}
template <class T, class M>
DHolder<T,M>::~DHolder()
{
    pull();
}
template <class T, class M>
DHolder<T,M>::DHolder(const DHolder& h)
{
    k.w = h.k.w;
    k.add_copy();
    k.setAccessMode(CloneMode);
}
template <class T, class M>
DHolder<T,M>::DHolder(DHolder& h)
{
    qDebug()<<"DHolder: copy by non-const ref";
    k.w = h.k.w;
    k.add_copy();
    k.setAccessMode(CloneMode);
}
template <class T, class M>
DHolder<T,M>& DHolder<T,M>::operator=(const DHolder& h)
{
    pull();
    k.w = h.k.w;
    k.add_copy();
    k.setAccessMode(ConstCloneMode);
}
template <class T, class M>
DHolder<T, M>& DHolder<T,M>::operator=(DHolder& h)
{
    pull();
    k.w = h.k.w;
    k.add_copy();
    k.setAccessMode(CloneMode);
}
template <class T, class M>
bool DHolder<T,M>::is_unique() const
{
    return k.is_unique();
}
template <class T, class M>
T* DHolder<T,M>::make_unique()
{
    if(!is_unique())
    {
        k.pull();
        k.hold_it(clone());
    }
}
template <class T, class M>
T* DHolder<T,M>::try_detach()
{
    void* p =  k.isDetachable() ? k.detach(clone()) : k.get();
    return reinterpret_cast<T*>(p);
}
template <class T, class M>
void DHolder<T,M>::setAccessMode(_dholder_modes m)
{
    if(k.setAccessMode(m)) k.detach(clone());
}


template <class T>
T& share(T& o)
{
    o.setAccessMode(ShareNowMode);
    return o;
}

#endif // DHOLDER_H

