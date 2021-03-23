#ifndef DARRAY_H
#define DARRAY_H

#include "DWatcher.h"
#include "dmem.h"

#include <QDebug>

enum DArrayWriteMode {Direct, Undirect, Auto};
template <class T, DArrayWriteMode WriteMode = Auto>
class DArray
{
    struct LargeType {enum {DirectPlacement = false};};
    struct SmallType {enum {DirectPlacement = true};};
    struct TrivialType {};
    struct ComplexType {};


    struct __metatype : std::conditional<   (WriteMode == Auto),
    typename std::conditional< (sizeof(T) > sizeof(void*)),  LargeType, SmallType>::type,
    typename std::conditional< (WriteMode == Undirect),  LargeType, SmallType>::type>::type,
                        std::conditional<  (std::is_trivial<T>::value), TrivialType, ComplexType>::type
      {};
public:
    struct complex_iterator;
    struct const_complex_iterator;
    typedef typename std::conditional< (__metatype::DirectPlacement), T*, complex_iterator >::type iterator;
    typedef typename std::conditional< (__metatype::DirectPlacement), const T*, const_complex_iterator >::type const_iterator;
    typedef typename std::conditional< (__metatype::DirectPlacement), T, T* >::type stored_type;
    typedef typename std::conditional< (__metatype::DirectPlacement), T const, T*const >::type const_stored_type;
    typedef stored_type* raw_iterator;
    typedef const_stored_type* const_raw_iterator;



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
        if(w->pull() == 0) {_destruct(__metatype()); delete data(); delete w;}
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
            DArray<T, WriteMode> tmp(a);
            tmp.swap(*this);
        }
        return *this;
    }

    struct complex_iterator
    {
        typedef T* pointer_t;
        complex_iterator() : p(nullptr){}
        complex_iterator(T** _p) : p(_p){}
        inline pointer_t ptr() {return *p;}
        inline operator T**() {return p;}
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
        typedef T*const pointer_t;
        const_complex_iterator() : p(nullptr){}
        const_complex_iterator(T*const* _p) : p(_p){}
        inline pointer_t ptr() {return *p;}
        inline operator T*const*() {return p;}
        inline const T& operator*() const {return **p;}
        inline const T* operator->() const {return *p;}
        inline const_complex_iterator& operator++(){++p; return *this;}
        inline const_complex_iterator operator++(int) {const_complex_iterator it(p); ++p; return it;}
        inline const_complex_iterator& operator--() {--p; return *this;}
        inline const_complex_iterator operator--(int){const_complex_iterator it(p); --p; return it;}
        inline bool operator==(const const_complex_iterator& it) const noexcept { return p==it.p;}
        inline bool operator!=(const const_complex_iterator& it) const noexcept { return p!=it.p;}
        inline const_complex_iterator operator+(int i) const {return const_complex_iterator(p+i);}
        inline const_complex_iterator operator-(int i) const {return const_complex_iterator(p-i);}
        T*const* p;
    };



    raw_iterator raw_begin() {detach(); return data()->t();}
    raw_iterator raw_end() {detach(); return data()->t() + data()->size;}
    const_raw_iterator raw_begin() const {return data()->t();}
    const_raw_iterator raw_end() const {return data()->t() + data()->size;}

    iterator begin() {detach(); return data()->t();}
    iterator end() {detach(); return data()->t() + data()->size;}
    iterator first() {detach(); return begin();}
    iterator last(){detach(); return data()->t() + data()->size - 1;}

    const_iterator begin() const {return data()->t();}
    const_iterator end() const {return data()->t() + data()->size;}
    const_iterator first() const {return begin();}
    const_iterator last() const {return data()->t() + data()->size - 1;}
//-------------------------------------------------------------------------------------------
public:
    void setMode(WatcherMode m)
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
    inline void make_unique()
    {
        w->refDown();
        w = new DDualWatcher(clone(), CloneWatcher);
    }
    inline bool is_unique() const
    {
        return w->is_unique();
    }
public:
    const char* type_size_name() const {return _type_size(__metatype());}
    const char* type_signature_name() const {return _type_signature(__metatype());}
    int size() const {return data()->size;}
//-------------------------------------------------------------------------------------------
    int cell_size() const {return sizeof(stored_type);}
    int reserved() const {return data()->alloc;}
    int available() const {return data()->alloc - data()->size;}
    bool empty() const {return !data()->size;}
    const T& constFront() const {return _get_ref(0);}
    const T& constBack() const {return _get_ref(data()->size-1);}
    const T& operator[](int i) const {return _get_ref(i);}
    const T& at(int i) const {if(i >= 0 && i < data()->size) return _get_ref(i);}

    int count(const T& t, int start_pos = 0, int end_pos = 0) const
    {
        if(start_pos >= end_pos) start_pos = end_pos = 0;
        auto b = begin() + start_pos;
        auto e = end_pos ? begin() + end_pos : end();
        int i=0;
        while( b != e ) if( *b++ == t ) ++i;
        return i;
    }
    const_iterator contain(const T& t) const
    {
        auto b = begin();
        auto e = end();
        while( b != e ) if(*b++ == t) return b;
        return nullptr;
    }
    const_iterator contain_within(const T& t, int start = 0, int end = 0) const
    {
        if(start >= end) start = end = 0;
        auto b = begin() + start;
        auto e = begin() + end;
        while( b != e ) if(*b++ == t) return b;
        return nullptr;
    }
    iterator contain(const T& t)
    {
        auto b = begin();
        auto e = end();
        while( b != e ) if(*b++ == t) return b;
        return nullptr;
    }
    iterator contain_within(const T& t, int start = 0, int end = 0)
    {
        if(start >= end) start = end = 0;
        auto b = begin() + start;
        auto e = begin() + end;
        while( b != e ) if(*b++ == t) return b;
        return nullptr;
    }

    void append(){detach();  _push(data()->_get_place(), __metatype());}
    void append(const T& t) {detach();  _push(t, data()->_get_place(), __metatype());}
    void append(const T& t, int n) {detach(); while(n--) append(t);}
    void append_line(const DArray<T, WriteMode>& from)
    {
        if(__metatype::DirectPlacement && std::is_trivial<T>::value)
        {
                if(available() < from.size()) reserve(  reserved() + (from.size() - available()) );
                memcpy(end(), from.constBegin(), from.size() * sizeof(stored_type));
                data()->size += from.size();
        }
        else
        {
            auto b = from.constBegin();
            auto e = from.constEnd();
            while( b != e )append(*b++);
        }
    }
    void reserve(int s) {detach(); data()->_reserve(s);}
    void remove(iterator it){detach();_remove(it, __metatype());}
    void remove_all() {detach(); _destruct(__metatype());}
    void remove_by_index(int i) {detach(); _remove(i, __metatype());}
    void remove(const T& v)
    {
        detach();
        auto b = begin();
        auto e = end();
        int i=0;
        while( b != e )
        {
            if( *b++ == v ) _remove(i, __metatype()); ++i;
        }
    }
    T& at(int i) {detach(); if(i >= 0 && i < data()->size) return _get_ref(i);}
    T& operator[](int i) {detach(); return _get_ref(i);}
    void clear() {detach(); *this = DArray();}
    T& front() {detach(); return _get_ref(0);}
    T& back() {detach(); return _get_ref(data()->size-1);}
    void push_front(const T& t){detach();_insert(t, 0);}
    void push_back(const T& t) {append(t);}
    void pop_back() {detach(); _remove(data()->size-1, __metatype());}
    void pop_front() {detach(); _remove(0, __metatype());}
    void replace(int i, const T& v) {if(i >= 0 && i < data()->size) {detach(); _get_ref(i) = v;}}
    void swap(DArray& with){std::swap(w, with.w);}
    void insert(const T& v, int pos) {detach(); _insert(v, pos);}

    void element_swap(int first, int second)
    {
        std::swap( *(data()->t() + first ), *(data()->t() + second) );
    }
    void element_swap(iterator first, iterator second)
    {
        if(__metatype::DirectPlacement) std::swap( *first, *second );
        else std::swap( *static_cast<T**>(first), *static_cast<T**>(second) );
    }
public:
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
            if( s > alloc || (s < alloc && s >= size) )
            {
                alloc = s;
                data = reget_mem((char*)data, _real_size( alloc ));
            }
        }
        void* _get_place()
        {
            _reserve_up();
            return reinterpret_cast<char*>(data) + _real_size(size++);
        }
        int   _real_size(int s) {return s*cell_size;}
        void  _reserve_up()
        {
            if(alloc == size)
            {
                alloc = ( size + 1 ) * 2;
                data = reget_mem((char*)data, _real_size( alloc ));
            }
        }
    };
    DDualWatcher* w;


    Data* clone()
    {
        Data* _d = new Data(sizeof(stored_type));
        _d->_reserve(data()->alloc);
        auto b_source = begin();
        auto e_source = end();
        while(b_source != e_source) _push(*b_source++, _d->_get_place(), __metatype());
        return _d;
    }
    void detach()
    {
        if(!w->is_unique())
        {
            if(w->is_share() && w->otherSideRefs())
            {
                w->disconnect(clone());
            }
            else if(w->is_clone())
            {
                w->refDown();
                if(w->sideRefs() == 0) w->disconnect();
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
    void _push(void* place, LargeType)
    {
        *reinterpret_cast<T**>(place) = new T;
    }
    void _push(const T& t, void* place, SmallType)
    {
        if(std::is_trivial<T>::value) *reinterpret_cast<T*>(place) = t;
        else new (place) T(t);
    }
    void _push(const T& t, void* place, LargeType)
    {
        *reinterpret_cast<T**>(place) = new T(t);
    }

    void _insert( const T& t, int i)
    {
           data()->_reserve_up();
           memmove((data()->t() + i + 1),(data()->t() + i), (data()->size - i) * sizeof(stored_type));
           _push(t, data()->t() + i, __metatype());
           ++data()->size;
    }
    void _remove( iterator it, SmallType)
    {
        if(!std::is_trivial<T>::value) it->~T();
        memmove( it,  it+1,  (end() - (it+1)) * sizeof(stored_type)  );
        --data()->size;
    }
    void _remove( iterator it, LargeType )
    {
        delete &*it;
        memmove( it,  it+1,  (end() - (it+1)) * sizeof(stored_type)  );
        --data()->size;
    }
    void _remove( int i, SmallType )
    {
        if(!std::is_trivial<T>::value) reinterpret_cast<T*>(data()->t() + i)->~T();
        memmove( (data()->t() + i),  (data()->t() + i+1),  (data()->size - i - 1) * sizeof(stored_type)  );
        --data()->size;
    }
    void _remove( int i, LargeType )
    {
        delete *reinterpret_cast<T**>(data()->t() + i);
        memmove( (data()->t() + i),  (data()->t() + i+1),  (data()->size - i - 1) * sizeof(stored_type)  );
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
        if(!std::is_trivial<T>::value) while(b != e) (--e)->~T();
        data()->size = 0;
    }
    void _destruct(LargeType)
    {
        auto b = data()->t();
        auto e = data()->t() + data()->size;
        while(b != e) delete *(--e);
        data()->size = 0;
    }
};


/*
 test block
    DArray<int> v(5);
    DArray<int> v2 = v;
//    v.setMode(ShareWatcher);
//    v2.setMode(ShareWatcher);
    v[0] = 111;
    v[1] = 222;
    v[2] = 333;
    v[3] = 444;
    v[4] = 555;
    v2.append(818);
    v.insert(777, 1);
    v.swap(v2);
    v.push_back(666);

    printf("v: size: %d reserved: %d available: %d cell_size: %d\n", v.size(), v.reserved(), v.available(), v.cell_size());
    printf("v2: size: %d reserved: %d available: %d cell_size: %d\n", v2.size(), v2.reserved(), v2.available(), v2.cell_size());
//    printf("empty: %d type_size_name: %s type_signature_name: %s\n", v.empty(), v.type_size_name(), v.type_signature_name());
    auto b = v.begin();
    auto e = v.end();
    while( b!= e ) printf("v: %d\n", *b++);
    for(int i=0;i!=v2.size();++i) printf("v2: %d\n", v2[i]);
    printf("front: %d back:%d\n", v.front(), v.back());
    printf("first: %d last: %d\n", *v.first(), *v.last());

    v2 = v1;
    v1.setMode(ShareWatcher);
    v2.setMode(ShareWatcher);
    v1[0] = 111;
    v1[1] = 222;
//    v1.setMode(CloneWatcher);
//    v2.setMode(CloneWatcher);
//    v2.make_unique();
    v2[0] = 333;


    qDebug()<<"v1 watcher:";
    qDebug()<<v1.w<<v1.w->mode()<<"refs:"<<v1.w->refs()<<"is clone:"<<v1.w->is_clone()<<"is share:"<<v1.w->is_share()<<"is unique:"<<v1.w->is_unique()
           <<"is other side:"<<v1.w->is_otherSide()<<"other side refs:"<<v1.w->otherSideRefs();
    qDebug()<<"v2 watcher:";
    qDebug()<<v2.w<<v1.w->mode()<<"refs:"<<v2.w->refs()<<"is clone:"<<v2.w->is_clone()<<"is share:"<<v2.w->is_share()<<"is unique:"<<v2.w->is_unique()
           <<"is other side:"<<v2.w->is_otherSide()<<"other side refs:"<<v2.w->otherSideRefs();

    auto b = v1.begin();
    auto e = v1.end();
    while( b != e ) printf("v1 value: %d\n", *b++);
    qDebug()<<"---------";
    b = v2.begin();
    e = v2.end();
    while( b != e ) printf("v2 value: %d\n", *b++);
 * */

#endif // DARRAY_H
