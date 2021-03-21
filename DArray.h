#ifndef DARRAY_H
#define DARRAY_H

#include "DWatcher.h"
#include "dmem.h"

struct FastWriteSlowRead { enum {FastWrite = true}; };
struct SlowWriteFastRead { enum {FastWrite = false}; };

template <class T, class WriteModel = FastWriteSlowRead>
class DArray
{
    struct LargeType {enum {DirectPlacement = false};}; //rename it: this mean data array contain pointer to tartget object
    struct SmallType {enum {DirectPlacement = true};}; //rename it: this mean data array contain real object
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
            DArray<T, WriteModel> tmp(a);
            tmp.swap(*this);
        }
        return *this;
    }

    struct complex_iterator
    {
        complex_iterator() : p(nullptr){}
        complex_iterator(T** _p) : p(_p){}
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
        const_complex_iterator() : p(nullptr){}
        const_complex_iterator(T*const* _p) : p(_p){}
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
    int available() const {return data()->alloc - data()->size;}
    bool empty() const {return !data()->size;}
    const T& constFront() const {return _get_ref(0);}
    const T& constBack() const {return _get_ref(data()->size-1);}
    const T& operator[](int i) const {return _get_ref(i);}
    const T& at(int i) const {if(i >= 0 && i < data()->size) return _get_ref(i);}

    int count(const T& t) const
    {
        auto b = constBegin();
        auto e = constEnd();
        int i=0;
        while( b != e ) if( *b++ == t ) ++i;
        return i;
    }
    void append(){detach();  _push(data()->_get_place(), __metatype());}
    void append(const T& t) {detach();  _push(t, data()->_get_place(), __metatype());}
    void append(const T& t, int n) {detach(); while(n--) append(t);}
    void append(const DArray<T, WriteModel>& from)
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
        auto b = constBegin();
        auto e = constEnd();
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
    void swap(DArray& with){detach();  std::swap(w, with.w);}
    void insert(const T& v, int pos) {detach(); _insert(v, pos);}
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
    void _push(void* place, LargeType)
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

    void _insert( const T& t, int i)
    {
           data()->_reserve_up();
           memcpy( (data()->t() + i + 1),(data()->t() + i), (data()->size - i) * sizeof(stored_type));
           _push(t, data()->t() + i, __metatype());
           ++data()->size;
    }
    void _remove( iterator it, SmallType)
    {
        if(!std::is_trivial<T>::value) it->~T();
        memcpy( it,  it+1,  (constEnd() - (it+1)) * sizeof(stored_type)  );
        --data()->size;
    }
    void _remove( iterator it, LargeType )
    {
        delete &*it;
        memcpy( it,  it+1,  (constEnd() - (it+1)) * sizeof(stored_type)  );
        --data()->size;
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


#endif // DARRAY_H
