#ifndef DARRAY_H
#define DARRAY_H

#include <QDebug>
#include "DWatcher.h"
#include "dmem.h"

struct FastWriteSlowRead { enum {FastWrite = true}; };
struct SlowWriteFastRead { enum {FastWrite = false}; };

template <class T, class WriteModel = FastWriteSlowRead>
class DArray
{
    struct LargeType {enum {DirectPlacement = false};};
    struct SmallType {enum {DirectPlacement = true};};
    struct TrivialType {};
    struct ComplexType {};


    struct __metatype : std::conditional<  (!WriteModel::FastWrite), SmallType,  typename std::conditional< (sizeof(T) > sizeof(void*)),  LargeType, SmallType>::type>::type,
                        std::conditional<  (std::is_trivial<T>::value), TrivialType, ComplexType>::type
      {};
public:
    struct complex_iterator;
    struct const_complex_iterator;
    typedef typename std::conditional< (__metatype::DirectPlacement), T*, complex_iterator>::type iterator;
    typedef typename std::conditional< (__metatype::DirectPlacement), const T*, const_complex_iterator>::type const_iterator;
    typedef typename std::conditional< (__metatype::DirectPlacement), T, T*>::type stored_type;

    DArray()
    {
        Data* d = new Data(sizeof(stored_type));
        w = new DDualWatcher(d, CloneWatcher);
    }
    DArray(int s)
    {
        Data* d = new Data(sizeof(stored_type));
        w = new DDualWatcher(d, CloneWatcher);
        while(s--) append();
    }
    ~DArray()
    {
        if(w->pull() == 0) {_destruct(__metatype()); delete w;}
    }

    DArray(const DArray& a)
    {
        w = a.w;
        w->refUp();
    }
    DArray& operator=(const DArray& a)
    {
        if(w != a.w)
        {
            DArray<T> tmp(a);
            tmp.swap(*this);
        }
        return *this;
    }

    struct complex_iterator
    {
        complex_iterator() : p(nullptr){}
        complex_iterator(T** _p) : p(_p){}
        inline T& operator*() const {return **p;}
        inline T* operator->() const {return *p;}
        inline complex_iterator& operator++(){++p; return *this;}
        inline complex_iterator operator++(int){complex_iterator it(p); ++p; return it;}
        inline complex_iterator& operator--(){--p; return *this;}
        inline complex_iterator operator--(int){complex_iterator it(p); --p; return it;}
        inline  bool operator==(const complex_iterator& it) const noexcept { return p==it.p; }
        inline  bool operator!=(const complex_iterator& it) const noexcept { return p!=it.p; }
        inline  bool operator==(const const_complex_iterator& it) const noexcept { return p==it.p; }
        inline  bool operator!=(const const_complex_iterator& it) const noexcept { return p!=it.p; }
        inline complex_iterator operator+(int i) const {return complex_iterator(p+i);}
        inline complex_iterator operator-(int i) const {return complex_iterator(p-i);}
        T** p;
    };
    struct const_complex_iterator
    {
        const_complex_iterator() : p(nullptr){}
        const_complex_iterator(T** _p) : p(_p){}
        inline const T& operator*() const {return **p;}
        inline const T* operator->() const {return *p;}
        inline const_complex_iterator& operator++(){++p; return *this;}
        inline const_complex_iterator operator++(int) {const_complex_iterator it(p); ++p; return it;}
        inline const_complex_iterator& operator--() {--p; return *this;}
        inline const_complex_iterator operator--(int){const_complex_iterator it(p); --p; return it;}
        inline  bool operator==(const const_complex_iterator& it) const noexcept { return p==it.p;}
        inline  bool operator!=(const const_complex_iterator& it) const noexcept { return p!=it.p;}
        inline const_complex_iterator operator+(int i) const {return const_complex_iterator(p+i);}
        inline const_complex_iterator operator-(int i) const {return const_complex_iterator(p-i);}
        T** p;
    };


    iterator begin() { return data()->t();}
    iterator end() {return data()->t() + data()->size;}
    const_iterator constBegin() const {return data()->t();}
    const_iterator constEnd() const {return data()->t() + data()->size;}
//-------------------------------------------------------------------------------------------
public:
    void setMode(WatcherMode m)
    {
        if(m != w->mode())
        {
            w->refDown();
            w = w->otherSide();
            w->refUp();
        }
    }

    inline void make_unique(){detach();}
public:
    const char* type_size_name() const {return _type_size(__metatype());}
    const char* type_signature_name() const {return _type_signature(__metatype());}
    int size() const {return data()->size;}
//-------------------------------------------------------------------------------------------
    int cell_size() const {return sizeof(stored_type);}
    int reserved() const {return data()->alloc;}
    bool empty() const {return !data()->size;}
    const T& constFirst() const {return _get_ref(0);}
    const T& constLast() const {return _get_ref(data()->size-1);}
    const T& operator[](int i) const {return _get_ref(i);}

    void append(){detach();   _push(data()->_get_place(), __metatype());}
    void append(const T& t) {detach();  _push(t, data()->_get_place(), __metatype());}
    void append(const T& t, int n) {detach(); while(n--) append(t);}
    void reserve(int s) {detach(); data()->_reserve(s);}
    void remove(int i) {detach(); _remove(i, __metatype());}
    T& operator[](int i) {detach(); return _get_ref(i, __metatype());}
    void clear() {detach(); *this = DArray();}
    T& first() {detach(); return _get_ref(0);}
    T& last() {detach(); return _get_ref(data()->size-1);}
    void push_back(const T& v) {detach(); append(v);}
    void replace(int i, const T& v) {if(i >= 0 && i < data()->size){detach(); _get_ref(i, __metatype()) = v;}}

    void swap(DArray<T>& with){std::swap(w, with.w);}
private:
    struct Data
    {
        Data(int cs) : data(nullptr), alloc(0), size(0), cell_size(cs){}
        ~Data() {free_mem(data);}
        void* data;
        int alloc;
        int size;
        int cell_size;

        stored_type* t()
        {return reinterpret_cast<stored_type*>(data);}
        const stored_type* t() const
        {return reinterpret_cast<const stored_type*>(data);}

        void  _reserve(int s)
        {
            if( s > alloc )
            {
                alloc = s;
                data = reget_mem(data, _real_size( alloc ));
            }
        }
        void* _get_place()
        {
            if(alloc == size) _reserve_up();
            return reinterpret_cast<char*>(data) + _real_size(size++);
        }
        int   _real_size(int s) {return s*cell_size;}
        void  _reserve_up()
        {
            alloc = ( size + 1 ) * 2;
            data = reget_mem(data, _real_size( alloc ));
        }

    };
    DDualWatcher* w;


    Data* clone()
    {
        Data* _d = new Data(sizeof(stored_type));
        _d->_reserve(data()->alloc);
        auto b_source = constBegin();
        auto e_source = constEnd();
        while(b_source != e_source) _push(*b_source++, _d->_get_place(), __metatype());
        return _d;
    }
    void detach()
    {
        if(!w->is_unique())
        {
            if(w->is_share() && w->otherSideRefs()) w->disconnect(clone());
            else if(w->is_clone())
            {
                w->refDown();
                w = new DDualWatcher(clone(), CloneWatcher);
            }
        }
    }
//---------------------------------------------------------------------------------------
    Data* data() {return reinterpret_cast<Data*>(w->d());}
    const Data* data() const {return reinterpret_cast<const Data*>(w->d());}

    const char* _type_size(LargeType) const {return "<Large Type>";}
    const char* _type_size(SmallType) const {return "<Small Type>";}
    const char* _type_signature(TrivialType) const {return "<Trivial Type>";}
    const char* _type_signature(ComplexType) const {return "<Complex Type>";}

    void _push(void* place, SmallType)
    {
        if(std::is_trivial<T>::value) *reinterpret_cast<T*>(place) = T();
        else new (place) T;
    }
    void _push( void* place, LargeType)
    {
        *reinterpret_cast<T**>(place) = new T;
    }
    void _push(const T& t, void* place, SmallType)
    {
        if(std::is_trivial<T>::value)
            *reinterpret_cast<T*>(place) = t;
        else
            new (place) T(t);
    }
    void _push(const T& t, void* place, LargeType)
    {
        *reinterpret_cast<T**>(place) = new T(t);
    }

    void _remove( int i, SmallType )
    {
        if(!std::is_trivial<T>::value) reinterpret_cast<T*>(data()->t() + i)->~T();
        memcpy( (data()->t() + i),  (data()->t() + i+1),  (data()->size - i - 1) * sizeof(stored_type)  );
        --data()->size;
    }
    void _remove( int i, LargeType )
    {
        delete *reinterpret_cast<T**>(data()->t() + i);
        memcpy( (data()->t() + i),  (data()->t() + i+1),  (data()->size - i - 1) * sizeof(stored_type)  );
        --data()->size;
    }
    const T& _get_ref(int i) const
    {
        if(__metatype::DirectPlacement) return *reinterpret_cast<const T*>(  data()->t() + i  );
        else return **reinterpret_cast<T*const*>(  data()->t() + i  );

    }
    T& _get_ref(int i)
    {
        if(__metatype::DirectPlacement) return *reinterpret_cast<T*>(  data()->t() + i  );
        else return **reinterpret_cast<T**>(  data()->t() + i  );
    }
    void _destruct(SmallType)
    {
        auto b = data()->t();
        auto e = data()->t() + data()->size;
        if(!std::is_trivial<T>::value)
            while(b != e) (--e)->~T();
        delete data();
    }
    void _destruct(LargeType)
    {
        auto b = data()->t();
        auto e = data()->t() + data()->size;
        while(b != e) delete *(--e);
        delete data();
    }
};


#endif // DARRAY_H
