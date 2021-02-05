#ifndef DHOLDER_H
#define DHOLDER_H
#include <mutex>
#include <QDebug>

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
    DHolder& operator=(DHolder& a) = delete;
    DHolder& operator=(const DHolder&& a) = delete;
    DHolder& operator=(DHolder&& a) = delete;
    //-----------------------------
    T* get(){return _target;}
    T& operator*() {return *_target;}
    T*  alloc();
    template <class ... Args>
    T* alloc(Args... a);
    void set(T* _src);
    void cpy(T& v);
    bool is_unique();
    T *make_unique();
    T *try_detach();

    void enable_watch() {_h->watch_it = true;}
    void disable_watch() {_h->watch_it = false;}
    const bool& is_watch() const {return _h->watch_it;}
    const int& holders() const {return _h->population;}


    enum CopyMode {ShareMode = 1, SaveMode = 0};
    void set_copy_mode(CopyMode);
private:
    struct _handler_t
    {
        bool watch_it;
        int population;
        std::mutex _mu;
    };

    enum ws{WatchIt = true, NoWatch = false};
    inline T* hold_it(T* _src,  ws watch)
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
        if(pop_down() == 0)
        {
            if(_h->watch_it && _target)
                delete _target;
            delete _h;
        }
        _h = nullptr;
        _target = nullptr;
    }
private:
    T* _target;
    _handler_t* _h;
    CopyMode _cm;
    void clear()
    {
        _target = nullptr;
        _h = nullptr;
        _cm = SaveMode;
    }
};

template <class T>
DHolder<T>::DHolder(bool create)
{
    clear();
    if(create)
        hold_it(new T, WatchIt);
    else
        hold_it(nullptr, NoWatch);
}
template <class T>
DHolder<T>::DHolder(T* _src)
{
    clear();
    hold_it(_src, NoWatch);
}
template <class T>
DHolder<T>::DHolder(const DHolder& a)
{
    clear();
    _cm = a._cm;
    _target = a._target;
    _h = a._h;
    pop_up();
}
template <class T>
DHolder<T>::DHolder(T& v)
{
    clear();
    hold_it(&v, NoWatch);
}
template <class T>
DHolder<T>& DHolder<T>::operator=(const DHolder<T>& a)
{
    if(this != &a)
    {
        pull();
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
    hold_it(new T(v), WatchIt);
}
template <class T>
bool DHolder<T>::is_unique()
{
    return _h->population == 1;
}
template <class T>
T *DHolder<T>::alloc()
{
    pull();
    return hold_it(new T, WatchIt);
}
template <class T>
template <class ... Args>
T *DHolder<T>::alloc(Args...a)
{
    pull();
    return hold_it(new T(a...), WatchIt);
}
template <class T>
void DHolder<T>::set(T* _src)
{
    pull();
    hold_it(_src, NoWatch);
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
        hold_it(new T(*get()), WatchIt);
    }
    return get();
}
template <class T>
T* DHolder<T>::try_detach()
{
    return _cm == SaveMode? make_unique():get();
}
template <class T>
void DHolder<T>::set_copy_mode(CopyMode cm)
{
    if(cm == SaveMode && _cm == ShareMode)
        make_unique();
    _cm = cm;
}
#endif // DHOLDER_H
