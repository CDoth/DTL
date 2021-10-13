#ifndef DARRAY_H
#define DARRAY_H

#include "DWatcher.h"
#include "dmem.h"

//#include <QDebug>
//host check
//123



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
        Data* d = new Data();
        w = new DDualWatcher(d, CloneWatcher);
    }
    explicit DArray(int s)
    {
        Data* d = new Data(sizeof(stored_type));
        w = new DDualWatcher(d, CloneWatcher);
        reserve(s);
        while(s--) append();
    }
    ~DArray()
    {
        if(w->pull() == 0) {_destruct(__metatype());/* delete data(); delete w;*/}
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

    const_iterator constBegin() const {return data()->t();}
    const_iterator constEnd() const {return data()->t() + data()->size;}
    const_iterator constFirst() const {return begin();}
    const_iterator constLast() const {return data()->t() + data()->size - 1;}
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
    bool empty() const {return data()->size == 0;}
    const T& constFront() const {return _get_ref(0);}
    const T& constBack() const {return _get_ref(data()->size-1);}
    const T& operator[](int i) const {return _get_ref(i);}
    const T* pointer(int i) const {detach(); return _get_ptr(i);}
    const T& at(int i) const {if(i >= 0 && i < data()->size) return _get_ref(i);}


    int count(const T& t) const
    {
        auto b = constBegin();
        auto e = constEnd();
        int i=0;
        while( b != e ) if( *b++ == t ) ++i;
        return i;
    }
    int count_within(const T& t, int start_pos, int end_pos) const
    {
        if(start_pos >= end_pos)
        {
            start_pos = 0;
            end_pos = 0;
        }
        auto b = constBegin() + start_pos;
        auto e = end_pos ? constBegin() + end_pos : constEnd();
        int i=0;
        while( b != e ) if( *b++ == t ) ++i;
        return i;
    }
    int index_of(const T &t)
    {
        auto b = constBegin();
        auto e = constEnd();
        int i=0;
        while( b != e )
        {
            if( *b++ == t ) return i;
            ++i;
        }
        return -1;
    }
    int index_of_within(const T &t, int start_pos, int end_pos)
    {
        if(start_pos >= end_pos)
        {
            start_pos = 0;
            end_pos = 0;
        }
        auto b = constBegin() + start_pos;
        auto e = end_pos ? constBegin() + end_pos : constEnd();
        int i=0;
        while( b != e )
        {
            if( *b++ == t ) return i;
            ++i;
        }
        return -1;
    }
    const_iterator contain(const T& t) const
    {
        auto b = begin();
        auto e = end();
        while( b != e )
        {
            if(*b == t)
                return b;
            ++b;
        }
        return e;
    }
    const_iterator contain_within(const T& t, int start = 0, int end = 0) const
    {
        if(start >= end) start = end = 0;
        auto b = begin() + start;
        auto e = begin() + end;
        while( b != e )
        {
            if(*b == t)
                return b;
            ++b;
        }
        return e;
    }
    iterator contain(const T& t)
    {
        auto b = begin();
        auto e = end();
        while( b != e )
        {
            if(*b == t)
                return b;
            ++b;
        }
        return e;
    }
    iterator contain_within(const T& t, int start = 0, int end = 0)
    {
        if(start >= end) start = end = 0;
        auto b = begin() + start;
        auto e = begin() + end;
        while( b != e )
        {
            if(*b == t)
                return b;
            ++b;
        }
        return e;
    }


    void append(){detach();  _push(data()->_get_place(), __metatype());}
    void append(const T &t) {detach();  _push(t, data()->_get_place(), __metatype());}
    void append(const T& t, int n) {detach(); while(n--) append(t);}
    void appendAt(int n)
    {
        append(T(), n);
    }
    void appendLine(const DArray<T, WriteMode>& from)
    {
        detach();
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
            while( b != e ) append(*b++);
        }
    }
    void appendLine(const T* d, int n)
    {
        detach();
        if(__metatype::DirectPlacement && std::is_trivial<T>::value)
        {
                if(available() < n) reserve(  reserved() + (n - available()) );
                copy_mem(end(), d, n);
                data()->size += n;
        }
        else
        {
            auto e = d + n;
            while( d != e ) append(*d++);
        }
    }
    void reform(int n)
    {
        if(n > size())
        {
            appendAt(n - size());
        }
        else if(n < size())
        {
            cutEnd(size() - n);
        }
    }

    void copy_value(const DArray &src)
    {
        detach();
        if(__metatype::DirectPlacement && std::is_trivial<T>::value)
        {
            copy_mem(begin(), src.constBegin(), std::min(size(), src.size()));
        }
        else
        {
            auto b_dst = begin();
            auto e_dst = end();
            auto b_src = src.constBegin();
            auto e_src = src.constEnd();

            while(b_dst != e_dst && b_src != e_src)
            {
                *b_dst++ = *b_src++;
            }
        }
    }
    void loadSource(const T *src, int n)
    {
        if(src == nullptr || n <= 0)
            return;
        detach();
        if(__metatype::DirectPlacement && std::is_trivial<T>::value)
        {
            if(n > size())
            {
                data()->_reserve_and_count(n);
            }
            if(n > 0)
            {
                copy_mem(begin(), src, n);
            }
        }
        else
        {
            if(n > size())
            {
                this->appendAt(n-size());
            }
            if(n > 0)
            {
                auto src_e = src + n;
                auto b = begin();
                while(src!=src_e)
                {
                    *b++ = *src++;
                }
            }
        }
    }
    void setSource(T *src, int n)
    {
        clear();
        data()->_set_source(src, n);
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
        while( b != e )
        {
            if( *b == v )
            {
                _remove(b, __metatype());
            }
            ++b;
        }
    }
    void removeFirst()
    {
        if(size())
        {
            detach();
            remove_by_index(0);
        }
    }
    void removeLast()
    {
        if(size())
        {
            detach();
            remove_by_index(size()-1);
        }
    }
    void cutEnd(int n)
    {
        if(size() && n > 0 && n <= size())
        {
            if(n == 1)
            {
                removeLast();
                return;
            }
            detach();
            _cut(size() - n, size()-1, __metatype());
        }
    }
    void cutBegin(int n)
    {
        if(size() && n > 0 && n <= size())
        {
            if(n == 1)
            {
                remove_by_index(0);
                return;
            }
            detach();
            _cut(0, n-1, __metatype());
        }
    }
    void cut(int start, int end)
    {
        if(size() && start >= 0 && start < size() && end >= 0 && end < size())
        {
            if(start == end)
            {
                remove_by_index(start);
                return;
            }
            detach();
            if(start > end)
                std::swap(start, end);
            _cut(start, end, __metatype());
        }
    }
    T& at(int i) {detach(); if(i >= 0 && i < data()->size) return _get_ref(i);}
    T& operator[](int i) {detach(); return _get_ref(i);}
    T* pointer(int i) {detach(); return _get_ptr(i);}
    void clear() {detach(); *this = DArray();}
    T& front() {detach(); return _get_ref(0);}
    T& back() {detach(); return _get_ref(data()->size-1);}
    void push_front(const T& t){detach();_insert(t, 0);}


    void push_back(const T& t) {append(t);}
    void push_back_debug(const T& t) {append_debug(t);}


    void pop_back() {detach(); _remove(data()->size-1, __metatype());}
    void pop_front() {detach(); _remove(0, __metatype());}
    void replace(int i, const T& v) {if(i >= 0 && i < data()->size) {detach(); _get_ref(i) = v;}}
    void swap(DArray& with){std::swap(w, with.w);}
    void insert(const T& v, int pos) {detach(); _insert(v, pos);}
    void insert_before(const T &v, const T &before)
    {
        auto b = constBegin();
        auto e = constEnd();
        int i=0;
        while( b != e )
        {
            if(*b++ == before)
            {
                insert(v, i);
                break;
            }
            ++i;
        }
    }
    void fill(const T& v)
    {
        detach();
        auto b = begin();
        auto e = end();
        while( b != e ) *b++ = v;
    }
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
        Data() : data(nullptr), alloc(0), size(0), outData(false)/*, cell_size(cs)*/{}
        ~Data() {free_mem(data);}
        stored_type *data;
        int alloc;
        int size;
        bool outData;


        stored_type* t()
        {return data;}
        const stored_type* t() const
        {return data;}

        stored_type* __cmpx_realloc(stored_type *_d, int _alloc_size, int _copy_size)
        {
            stored_type *_p = get_zmem<stored_type>(_alloc_size);
            auto _b = _d;
            auto _e = _d + _copy_size;
            while(_b != _e)
            {
                new (_p) stored_type(*_b);
                _b->~stored_type();
                ++_p;
                ++_b;
            }
            free_mem(_d);
            return _p - _copy_size;
        }
        void _reserve_and_count(int s)
        {
            if(_reserve(s) == s)
                size = alloc;
        }
        int  _reserve(int s)
        {
            if( s > alloc )
            {
                alloc = s;
                if(__metatype::DirectPlacement && !std::is_trivial<T>::value && size > 0)
                {
                    data = __cmpx_realloc(data, alloc, size);
                }
                else
                {
                    data = reget_mem(data, alloc);
                }
                return s;
            }
            return 0;
        }
        int _reserve_down(int s)
        {
            if( s < alloc && s >= size )
            {
                alloc = s;
                if(__metatype::DirectPlacement && !std::is_trivial<T>::value && size > 0)
                {
                    data = __cmpx_realloc(data, alloc, size);
                }
                else
                {
                    data = reget_mem(data, alloc);
                }
                return s;
            }
            return 0;
        }
        void* _get_place()
        {
            _reserve_up();
            return data + size++;
        }
        void  _reserve_up()
        {
            if(alloc == size)
            {
                alloc = ( size + 1 ) * 2;
                if(__metatype::DirectPlacement && !std::is_trivial<T>::value && size > 0)
                {
                    data = __cmpx_realloc(data, alloc, size);
                }
                else
                {
                    data = reget_mem(data, alloc);
                }
            }
        }
        void _set_source(stored_type *_d, int _size)
        {
            data = _d;
            size = _size;
            alloc = size;
            outData = true;
        }
    };
    DDualWatcher* w;


    Data* clone()
    {
        Data* _d = new Data();
        _d->_reserve(data()->alloc);
        auto b_source = constBegin();
        auto e_source = constEnd();
        while(b_source != e_source) _push(*b_source++, _d->_get_place(), __metatype());
        return _d;
    }
    void detach()
    {
        if(!w->is_unique() || data()->outData)
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
    void detach_debug()
    {
//        qDebug()<<"DArray::detach:"<<w->refs()<<this;
        if(!w->is_unique())
        {
            if(w->is_share() && w->otherSideRefs())
            {
//                qDebug()<<"---- disconnect";
                w->disconnect(clone());
            }
            else if(w->is_clone())
            {
//                qDebug()<<"---- clone"<<w->sideRefs();
                w->refDown();
                if(w->sideRefs() == 0) w->disconnect();
                w = new DDualWatcher(clone(), CloneWatcher);
            }
        }
    }
//---------------------------------------------------------------------------------------
    Data* data() {return reinterpret_cast<Data*>(w->d());}
    const Data* data() const {return reinterpret_cast<const Data*>(w->d());}

    const char* _type_size_name(LargeType) const {return "<Large Type>";}
    const char* _type_size_name(SmallType) const {return "<Small Type>";}
    const char* _type_signature_name(TrivialType) const {return "<Trivial Type>";}
    const char* _type_signature_name(ComplexType) const {return "<Complex Type>";}

    void _push(void* place, SmallType)
    {
        if(std::is_trivial<T>::value)
        {
            *reinterpret_cast<T*>(place) = T();
        }
        else
        {
            new (place) T;
        }
    }
    void _push(void* place, LargeType)
    {
//        if(__metatype::DirectPlacement)
//        {
//            new (place) T;
//        }
//        else
            *reinterpret_cast<T**>(place) = new T;
    }
    void _push(const T& t, void* place, SmallType)
    {
        if(std::is_trivial<T>::value) *reinterpret_cast<T*>(place) = t;
        else new (place) T(t);
    }
    void _push_debug(const T& t, void* place, SmallType)
    {
        if(std::is_trivial<T>::value)
        {
//            qDebug()<<"_push trivial for array:"<<this<<"to:"<<place;

            *reinterpret_cast<T*>(place) = t;
        }
        else
        {
//            qDebug()<<"_push complex for array:"<<this<<"to:"<<place<<"prev value: 0:"<<begin()<<"1:"<<begin() + 1;

            new (place) T(t);
        }
    }
    void _push(const T& t, void* place, LargeType)
    {
        *reinterpret_cast<T**>(place) = new T(t);
    }

    void _insert( const T& t, int i)
    {
        data()->_reserve_up();
        _move_data((data()->t() + i + 1),
                   (data()->t() + i),
                   (data()->size - i));
        _push(t, data()->t() + i, __metatype());
        ++data()->size;
    }
    void _remove( iterator it, SmallType)
    {
        if(!std::is_trivial<T>::value) it->~T();
        _move_data( it, it+1, (end() - (it+1)));
        --data()->size;
    }
    void _remove( iterator it, LargeType )
    {
        delete &*it;
        _move_data( it, it+1, (end() - (it+1)));
        --data()->size;
    }
    void _remove( int i, SmallType )
    {
        if(!std::is_trivial<T>::value) reinterpret_cast<T*>(data()->t() + i)->~T();
        _move_data( (data()->t() + i),  (data()->t() + i+1),  (data()->size - i - 1) );
        --data()->size;
    }
    void _remove( int i, LargeType )
    {
        delete *reinterpret_cast<T**>(data()->t() + i);
        _move_data( (data()->t() + i),  (data()->t() + i+1),  (data()->size - i - 1)  );
        --data()->size;
    }
    void _cut( int start, int end, SmallType)
    {
        if(__metatype::DirectPlacement)
        {
            if(!std::is_trivial<T>::value)
            {
                auto b = data()->t() + start;
                auto e = data()->t() + end;
                while(b != e)
                {
                    (--e)->~T();
                }
            }
        }
        _move_data(data()->t() + start,
                   data()->t() + end + 1,
                   data()->size - end);

        data()->size = data()->size - (end-start) - 1;
    }
    void _cut( int start, int end, LargeType)
    {
        auto b = data()->t() + start;
        auto e = data()->t() + end;
        while(b != e) delete *(--e);

        _move_data(data()->t() + start,
                   data()->t() + end + 1,
                   data()->size - end);

        data()->size = data()->size - (end-start) - 1;
    }
    void _move_data(stored_type *dst, stored_type *src, int n)
    {
        if(__metatype::DirectPlacement)
            placementNewMemMove(dst, src, n);
        else
            memmove(dst, src, n * sizeof(stored_type));
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
    const T* _get_ptr(int i) const
    {
        if(i >= 0 && i < data()->size)
        {
            if(__metatype::DirectPlacement) return reinterpret_cast<const T*>(data()->t() + i);
            else return *reinterpret_cast<T*const*>(data()->t() + i);
        }
        else
            return nullptr;
    }
    T* _get_ptr(int i)
    {
        if(i >= 0 && i < data()->size)
        {
            if(__metatype::DirectPlacement) return reinterpret_cast<T*>(data()->t() + i);
            else return *reinterpret_cast<T**>(data()->t() + i);
        }
        else
            return nullptr;
    }
    void _destruct(SmallType)
    {
        if(data()->outData)
            return;
        auto b = data()->t();
        auto e = data()->t() + data()->size;
        if(!std::is_trivial<T>::value)
        {
            while(b != e)
            {
//                qDebug()<<"___________________ delete for:"<<(e-1);
                (--e)->~T();
            }
        }
        data()->size = 0;
    }
    void _destruct(LargeType)
    {
        if(data()->outData)
            return;
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
