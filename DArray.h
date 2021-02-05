#ifndef DARRAY_H
#define DARRAY_H

#include <mutex>
#include <QDebug>

template <class T>
class DArray
{
public:
    DArray();

    explicit DArray(int s);
    DArray(int s, const T&);
    DArray(T* _src, int s);

    ~DArray();
    //-----------------------------
    DArray(const DArray& a);
    DArray(DArray& a) = delete;
    DArray(const DArray&& a) = delete;
    DArray(DArray&& a) = delete;

    DArray& operator=(const DArray& a);
    DArray& operator=(DArray& a) = delete;
    DArray& operator=(const DArray&& a) = delete;
    DArray& operator=(DArray&& a) = delete;
    //-----------------------------
    typedef T* iterator;
    typedef const T* const_iterator;

    iterator begin() const {return h->data;}
    iterator end() const {return h->data + h->size;}

    T& operator[](int i) {return h->data[i];}

    void reserve(int s);
    void reserve_up(int s);

    int  mount(const T&);
    int  mount();
    void push_back(const T&);
    void push_back();

    void alloc(int s, const T&);
    void alloc(int s);
    void set(T* _src, int s);

    void enable_watch() {h->watch_it = true;}
    void disable_watch() {h->watch_it = false;}
    bool is_watch() const {return h->watch_it;}
    int holders() const {return h->refs;}
    int size() const {return h->size;}
    void cpy(const DArray& _from);
    void fill(const T&);
    void fill_reserved(const T&);

    T *make_unique();
    bool is_unique() const;
    enum DetachMode {Detach = 1, NoDetach = 0};
    T* try_detach();
    void set_detach_mode(bool);

private:
    struct handler
    {
        T* data;
        int size;
        int reserved;

        int refs;

        bool is_c_mem;
        bool watch_it;

        std::mutex mu;
    };
    handler* h;
    DetachMode dm = Detach;


    inline void hold_it(T* _src, int s, bool watch)
    {
        h = new handler;
        h->data = _src;
        h->size = s;
        h->reserved = s;
        h->is_c_mem = false;
        h->watch_it = watch;
        h->refs = 1;
    }
    inline int ref_down()
    {
        h->mu.lock();
        --h->refs;
        h->mu.unlock();
        return h->refs;
    }
    inline int ref_up()
    {
        h->mu.lock();
        ++h->refs;
        h->mu.unlock();
        return h->refs;
    }
    inline void pull()
    {
        if(ref_down() == 0)
        {
            if(h->watch_it && h->data)
            {
                if(h->is_c_mem)
                {
                    for(auto it = begin(); it != end(); it++) it->~T();
                    free (h->data);
                }
                else delete[] h->data;
                h->data = nullptr;
            }
            delete h;
            h = nullptr;
        }
    }

    T* _mem(const int& s)
    {
        return (T*)malloc(sizeof(T) * s);
    }
};

template <class T>
DArray<T>::DArray()
{
    hold_it(nullptr, 0, false);
}
template <class T>
DArray<T>::DArray(int s)
{
    hold_it(new T[s], s, true);
}
template <class T>
DArray<T>::DArray(int s, const T& v)
{
    hold_it(_mem(s), s, true);
    h->is_c_mem = true;
    T* it = begin();
    while(it != end()) new (it++) T(v);
}
template <class T>
DArray<T>::DArray(T* _src, int s)
{
    hold_it(_src, s, false);
}
template <class T>
void DArray<T>::alloc(int s)
{
    pull();
    hold_it(new T[s], s, true);
}
template <class T>
void DArray<T>::alloc(int s, const T& v)
{
    pull();
    hold_it(_mem(s), s, true);
    h->is_c_mem = true;
    T* it = begin();
    while(it != end()) new (it++) T(v);
}
template <class T>
void DArray<T>::reserve(int s)
{
    pull();
    hold_it(_mem(s), 0, true);
    h->is_c_mem = true;
    h->reserved = s;
}
template <class T>
void DArray<T>::reserve_up(int s)
{
    hold_it((T*)realloc(h->data, sizeof(T) * (h->size + s)), h->size, true);
    h->is_c_mem = true;
    h->reserved = h->size + s;
}
template <class T>
int DArray<T>::mount()
{
    if(h->size < h->reserved)
    {
        new (end()) T;
        ++h->size;
    }
    return h->reserved - h->size;
}
template <class T>
int DArray<T>::mount(const T& v)
{
    if(h->size < h->reserved)
    {
        new (end()) T(v);
        ++h->size;
    }
    return h->reserved - h->size;
}
template <class T>
void DArray<T>::push_back()
{
    if(h->size == h->reserved) reserve_up(1);
    mount();
}
template <class T>
void DArray<T>::push_back(const T& v)
{
    if(h->size == h->reserved) reserve_up(1);
    mount(v);
}
template <class T>
void DArray<T>::set(T* _src, int s)
{
    pull();
    hold_it(_src, s, false);
}
template<class T>
void DArray<T>::fill(const T &v)
{
    iterator it = begin();
    while(it != end()) *it++ = v;
}
template <class T>
void DArray<T>::fill_reserved(const T& v)
{
    while(mount(v));
}
template <class T>
DArray<T>::DArray(const DArray& a)
{
    h = a.h;
    ref_up();
}
template <class T>
DArray<T>& DArray<T>::operator=(const DArray<T>& a)
{
    if(this != &a)
    {
        pull();
        h = a.h;
        ref_up();
    }
    return *this;
}
template <class T>
DArray<T>::~DArray()
{
    pull();
}
template <class T>
void DArray<T>::cpy(const DArray& _from)
{
    if(h->data && _from._data && h->size == _from._size)
    {
        memcpy(h->data, _from._data, sizeof(T) * h->size);
    }
}
template <class T>
T* DArray<T>::make_unique()
{
    if(is_unique())
    {
        T* load_to = _mem(h->size);
        auto to_b = load_to;
        auto b = begin();
        while( b != end() ) new (to_b++) T(*b++);
        int size = h->size;
        ref_down();
        hold_it(load_to, size, true);
        h->is_c_mem = true;
    }
    return begin();
}
template <class T>
T* DArray<T>::try_detach()
{
    return dm == Detach? make_unique() : begin();
}
template <class T>
bool DArray<T>::is_unique() const
{
    return h->refs == 1;
}
template <class T>
void DArray<T>::set_detach_mode(bool m)
{
    if(dm == NoDetach && m == true) make_unique();
    dm = m;
}

#endif // DARRAY_H
