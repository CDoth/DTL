#ifndef DHOLDER_H
#define DHOLDER_H
#include <mutex>
#include <QDebug>
#include "dmem.h"

template <class T>
class DHolder
{
public:
    explicit DHolder(bool create = false);
    explicit DHolder(T* _src);
    explicit DHolder(T& v);
    ~DHolder();
    //-----------------------------
    DHolder(const DHolder& a);
    DHolder(DHolder& a) = delete;
    DHolder(const DHolder&& a) = delete;
    DHolder(DHolder&& a) = delete;

    DHolder& operator=(const DHolder& a);
    DHolder& operator=(DHolder& a);
    DHolder& operator=(const DHolder&& a) = delete;
    DHolder& operator=(DHolder&& a) = delete;
    //-----------------------------

    T* get(){return _target;}
    T& operator*() {return *_target;}
    T* alloc(const T&);
    T* alloc();
    template <class ... Args>
    T* alloc(const Args&...);

    void set(T* _src, bool watch_it = false);
    void cpy(T& v);
    bool is_unique();
    T *make_unique();
    T *try_detach();

    void enable_watch() {_h->watch_it = true;}
    void disable_watch() {_h->watch_it = false;}
    const bool& is_watch() const {return _h->watch_it;}
    const int& holders() const {return _h->population;}

    void setShareCopyable(bool detach = false);
    void setSaveCopyable(bool detach = false);
    bool is_share() {return _cm==ShareMode;}

    void clear() {pull();}
private:
    struct _handler_t
    {
        bool watch_it;
        int population;
        std::mutex _mu;
    };


    struct late_pull
    {
        late_pull(_handler_t* _h, T* _t) : h(_h), t(_t){}
        ~late_pull()
        {
            if(h && pd() == 0)
            {
                if(h->watch_it && t)
                    delete t;
                delete h;
            }
        }

        int pd()
        {
            h->_mu.lock();
            --h->population;
            h->_mu.unlock();
            return h->population;
        }
        _handler_t* h;
        T* t;
    };

    inline T* hold_it(T* _src,  bool watch)
    {
        _target = _src;
        _h = new _handler_t;
        _h->watch_it = (bool)watch;
        _h->population = 1;
        return _target;
    }
    inline int pop_down()
    {
        _h->_mu.lock();
        --_h->population;
        _h->_mu.unlock();
        return _h->population;
    }
    inline int pop_up()
    {
        _h->_mu.lock();
        ++_h->population;
        _h->_mu.unlock();
        return _h->population;
    }
    inline void pull()
    {
        if(_h && pop_down() == 0)
        {
            if(_h->watch_it && _target)
                delete _target;
            delete _h;
        }
        _h = nullptr;
        _target = nullptr;
    }
private:
    enum CopyMode {ShareMode = 1, SaveMode = 0};
    T* _target;
    _handler_t* _h;
    CopyMode _cm;
    void reinit()
    {
        _target = nullptr;
        _h = nullptr;
        _cm = SaveMode;
    }
};

template <class T>
DHolder<T>::DHolder(bool create)
{
    reinit();
    if(create)
        hold_it(new T, true);
    else
        hold_it(nullptr, false);
}
template <class T>
DHolder<T>::DHolder(T* _src)
{
    reinit();
    hold_it(_src, true);
}
template <class T>
DHolder<T>::DHolder(const DHolder& a)
{
    reinit();
    _cm = a._cm;
    _target = a._target;
    _h = a._h;
    pop_up();
}
template <class T>
DHolder<T>::DHolder(T& v)
{
    reinit();
    hold_it(new T(v), true);
}
template <class T>
DHolder<T>& DHolder<T>::operator=(const DHolder<T>& a)
{
    if(this != &a)
    {
        pull();
        _cm = a._cm;
        _target = a._target;
        _h = a._h;
        pop_up();
    }
    return *this;
}
template <class T>
DHolder<T>& DHolder<T>::operator=(DHolder<T>& a)
{
    if(this != &a)
    {
        pull();
        _cm = a._cm;
        _target = a._target;
        _h = a._h;
        pop_up();
    }
    return *this;
}
template <class T>
void DHolder<T>::cpy(T& v)
{
    pull();
    hold_it(new T(v), true);
}
template <class T>
bool DHolder<T>::is_unique()
{
    return _h->population == 1;
}
template <class T>
T *DHolder<T>::alloc(const T& from)
{
    pull();
    return hold_it(new T(from), true);
}
template <class T>
T* DHolder<T>::alloc()
{
    pull();
    return hold_it(new T, true);
}
template <class T>
template <class ... Args>
T* DHolder<T>::alloc(const Args& ... a)
{
    pull();
    return hold_it(new T(a...), true);
}
template <class T>
void DHolder<T>::set(T* _src, bool watch_it)
{
    pull();
    hold_it(_src, watch_it);
}
template <class T>
DHolder<T>::~DHolder<T>()
{
    pull();
}
template <class T>
T* DHolder<T>::make_unique()
{
    if(!this->is_unique())
    {
        qDebug()<<"DHolder: call: make_unique"<<this;
        pop_down();
        hold_it(new T(*get()), true);
    }
    return get();
}
template <class T>
T* DHolder<T>::try_detach()
{
    return _cm == SaveMode? make_unique():get();
}
template <class T>
void DHolder<T>::setShareCopyable(bool detach)
{
    if(_cm == SaveMode && detach)
        make_unique();
    _cm = ShareMode;
}
template <class T>
void DHolder<T>::setSaveCopyable(bool detach)
{
    if(_cm == ShareMode && detach)
        make_unique();
    _cm = SaveMode;
}
#endif // DHOLDER_H
