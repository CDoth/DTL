#ifndef DARRAY_H
#define DARRAY_H

#include "DWatcher.h"
#include "dmem.h"

//#include <QDebug>
//host check
//123



enum DArrayMemManager {Direct, Undirect, Auto};
template <class T, DArrayMemManager memType>
class DArray;
template <class T, DArrayMemManager memType>
class DReadArray;



template <class T, DArrayMemManager memType>
class DArrayData {
    friend class DArray<T, memType>;
    friend class DReadArray<T, memType>;
    enum {
        __trivial = std::is_trivial<T>::value,
        __trivial_copy = std::is_trivially_copyable<T>::value,
        __trivial_destruct = std::is_trivially_destructible<T>::value
    };
    struct _direct_placement_layer { enum {direct_placement = 1}; };
    struct _undirect_placement_layer { enum {direct_placement = 0}; };

    // undirect:
    // 1. sizeof(T) > sizeof(void*)
    // 2. sizeof(T) <= sizeof(void*) && (__trivial_copy == false) (__trivial == true/false)
    // 3. UndirectMode

    // direct:
    // 1. sizeof(T) <= sizeof(void*) && (__trivial_copy == true) (__trivial  == true)
    // 2. DirectMode (__trivial  == true)

    struct __metatype : std::conditional
    < (memType == Auto),
    // Auto mode:
        typename std::conditional

        < ( sizeof(T) > sizeof(void*) ),
//            // Large types -> undirect
            _undirect_placement_layer,
//            // Small types:
            typename std::conditional
            <__trivial_copy,
//                // trivially copyable -> direct or cpx direct
                _direct_placement_layer,
//                // no trivially copyable -> undirect
                _undirect_placement_layer
            >::type
        > ::type,

    // Specific modes:
        typename std::conditional
        <(memType == Direct),
            _direct_placement_layer, _undirect_placement_layer
        >::type

//    _undirect_placement_layer


    > ::type

//            _direct_placement_layer

    {};


    struct complex_iterator;
    struct const_complex_iterator;



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

public:
    const char *placement() const {
        if(__metatype::direct_placement) {
            return "Direct placement";
        } else {
            return "Undirect placement";
        }
    }
    void info() const {
        std::cout << "Type size: " << sizeof(T)
                  << " placement: " << placement() << " [" << (int)__metatype::direct_placement << "]"
                  << " trvial: " << __trivial
                  << " tr copy: " << __trivial_copy
                  << " tr destr: " << __trivial_destruct

                  << std::endl;
    }

    typedef typename std::conditional< (__metatype::direct_placement), T*, complex_iterator >::type iterator;
    typedef typename std::conditional< (__metatype::direct_placement), const T*, const_complex_iterator >::type const_iterator;
    typedef typename std::conditional< (__metatype::direct_placement), T, T*>::type StoredType;
    typedef typename std::conditional< (__metatype::direct_placement), const T, const T*>::type ConstStoredType;
    typedef StoredType* rawIterator;
    typedef ConstStoredType* constRawIterator;


public:

    DArrayData() :
        data(nullptr),
        alloced(0),
        size(0),
        outterData(false)
    {}
    DArrayData(const DArrayData &d) :
    data(nullptr),
    alloced(0),
    size(0),
    outterData(false)
    { _force_data(d); }
    DArrayData(DArrayData &d)  :
    data(nullptr),
    alloced(0),
    size(0),
    outterData(false)
    { _force_data(d); }
    DArrayData& operator=(const DArrayData &d) { _force_data(d); return *this;}
    DArrayData& operator=(DArrayData &d) { _force_data(d); return *this;}
    ~DArrayData() { _dropAll(); }
private:
    StoredType *data;
    int alloced;
    int size;
    bool outterData;
private:

    void _set_source(T *src, int srcSize) {
        _dropAll();
        data = src;
        size = srcSize;
        alloced = srcSize;
    }
    void _set_const_source(const T *src, int srcSize) {
        _dropAll();
        data = src;
        size = srcSize;
        alloced = srcSize;
        outterData = true;
    }

    bool readOnly() const {return outterData;}
    int _available() const {return alloced - size;}
    int _size() const {return size;}
    int _capacity() const {return alloced;}
    iterator _begin() {return data;}
    const_iterator _begin() const {return data;}
    iterator _end() {return data + size;}
    const_iterator _end() const {return data + size;}

    void __reserve(int s, _direct_placement_layer) {

//        std::cout << "__reserve: size: " << s
//                  << " placement: " << placement()
//                  << std::endl;

        if(!__trivial_copy || !__trivial_destruct) {

            StoredType *p = get_zmem<StoredType>(s);
            StoredType *_p = p;
            auto b = data;
            auto e = data + size;
            while(b!=e) {
                new (_p) T(*b);
                b->~T();
                ++_p; ++b;
            }
            free_mem(data);
            data = p;
        } else {
            data = reget_mem(data, s);
        }
        alloced = s;
    }
    void __reserve(int s, _undirect_placement_layer) {



        data = reget_mem(data, s);
        alloced = s;

//        std::cout << "__reserve: reserve size: " << s
//                  << " inner size: " << size
//                  << " alloced: " << alloced
//                  << " placement: " << placement()
//                  << std::endl;
    }
    void __reserveUp() {
        if(alloced == size) {
            int s = ( size + 1 ) * 2;
            __reserve(s, __metatype());
        }
    }
    void __reserveUp(int s) {
        __reserve(alloced + s, __metatype());
    }
    StoredType *__getPlace() {
        __reserveUp();
        return data + size++;
    }
    void _reserve(int s) {
        __reserve(s, __metatype());
    }

private:
    T& __ref(int i, _direct_placement_layer) {return data[i];}
    T& __ref(int i, _undirect_placement_layer) {return *data[i];}
    const T& __ref(int i, _direct_placement_layer) const {return data[i];}
    const T& __ref(int i, _undirect_placement_layer) const {return *data[i];}
    T* __ptr(int i, _direct_placement_layer) {return data + i;}
    T* __ptr(int i, _undirect_placement_layer) {return data[i];}
    const T* __ptr(int i, _direct_placement_layer) const {return data + i;}
    const T* __ptr(int i, _undirect_placement_layer) const {return data[i];}

    T& _ref(int i) {return __ref(i, __metatype());}
    const T& _ref(int i) const {return __ref(i, __metatype());}
    T* _ptr(int i) {return __ptr(i, __metatype());}
    const T* _ptr(int i) const {return __ptr(i, __metatype());}



    void _moveObjects(StoredType *dst, StoredType *src, int n) {
        placementNewMemMove(dst, src, n);
    }
    void _copy_data(const DArrayData &d) {
        if(__metatype::direct_placement && __trivial_copy) {
            copy_mem(data, d.data, std::min(size, d.size));
        } else {
            int s = std::min(size, d.size);
            auto b = data;
            auto e = data + s;
            auto b_src = d.data;
            while(b!=e) {
                _set_value(b++, *b_src++, __metatype());
            }
        }
    }
    void _copy_data(const T *src, int srcSize) {
        if(__metatype::direct_placement && __trivial_copy) {
            copy_mem(data, src, std::min(size, srcSize));
        } else {
            int s = std::min(size, srcSize);
            auto b = data;
            auto e = data + s;
            auto b_src = src;
            while(b!=e) {
                _set_value(b++, *b_src++, __metatype());
            }
        }
    }
    void _force_data(const DArrayData &d) {
        _dropAll();
        __reserve(d.size, __metatype());
        _push_d(d);
        size = d.size;
    }
    void _force_data(const T *src, int srcSize) {
        _dropAll();
        __reserve(srcSize, __metatype());
        _push(src, srcSize);
        size = srcSize;
    }
    int _index_of(const T &t, int from = 0, int to = 0) const {
        if(from > to)
            std::swap(from, to);
        to = to ? to : size;
        int i = 0;
        auto b = _begin() + from;
        auto e = _begin() + to;
        while(b!=e) {
            if(*b++ == t) {
                return i + from;
            }
            ++i;
        }
        return -1;
    }
    int _count(const T &t, int from = 0, int to = 0) const {
        if(from > to)
            std::swap(from, to);
        to = to ? to : size;
        int c = 0;
        auto b = _begin() + from;
        auto e = _begin() + to;
        while(b!=e) {
            if(*b++ == t) ++c;
        }
        return c;
    }
    bool _contain(const T &t, int from = 0, int to = 0) const {
        if(from > to)
            std::swap(from, to);
        to = to ? to : size;
        auto b = _begin() + from;
        auto e = _begin() + to;
        while(b!=e) {
            if(*b++ == t) return true;
        }
        return false;
    }
    void _remove_by_index(int i) {
        _destruct(i, __metatype());
        _moveObjects( (data + i),  (data + i + 1),  (size - i - 1) );
        --size;
    }
    void _remove(const T &t) {
        int i = _index_of(t);
        if(i != -1) _remove_by_index(i);
    }
    void _drop() {
        _destructAll(__metatype());
        zero_mem(data, size);
        size = 0;
    }
    void _cut(int cutBegin, int cutEnd) {
        _destruct(cutBegin, cutEnd, __metatype());
        _moveObjects(data + cutBegin, data + cutEnd, size - cutEnd);
        size -= (cutEnd-cutBegin);
    }
    void _insert(int insertTo) {
        __reserveUp();
        _moveObjects( (data + insertTo + 1), (data + insertTo), (size - insertTo));
        _push(data + insertTo, __metatype());
    }
    void _insert(int insertTo, const T &src) {
        __reserveUp();
        _moveObjects( (data + insertTo + 1), (data + insertTo), (size - insertTo));
        __push(data + insertTo, src, __metatype());
        ++size;
    }
    void _relocate(int dstPos, int srcPos, int n) {
        DArrayData _d;
        _d._reserve(n);
        placementNewMemCopy(_d.data, data + srcPos, n);
        if(srcPos > dstPos) {
//            std::cout << "_relocate (1): "
//                      << " dstPos: " << dstPos
//                      << " srcPos: " << srcPos
//                      << " n: " << n
//                      << " move from: " << dstPos
//                      << " to: " << dstPos + n
//                      << " size: " << srcPos - dstPos
//                      << std::endl;

            placementNewMemMove(data + dstPos + n, data + dstPos, srcPos - dstPos);
        } else {
//            std::cout << "_relocate (2): "
//                      << " dstPos: " << dstPos
//                      << " srcPos: " << srcPos
//                      << " n: " << n
//                      << " move from: " << srcPos + n
//                      << " to: " << srcPos
//                      << " size: " << dstPos - srcPos
//                      << std::endl;
            placementNewMemMove(data + srcPos, data + srcPos + n, dstPos - srcPos);
        }
        placementNewMemCopy(data + dstPos, _d.data, n);
    }
    void _swap(int f, int s) {
        std::swap(data[f], data[s]);
    }

    void _set_value(int i, const T &t, _direct_placement_layer) { data[i] = t; }
    void _set_value(int i, const T &t, _undirect_placement_layer) { *data[i] = t;}
    void _set_value(StoredType *pos, const T &t, _direct_placement_layer) { *pos = t; }
    void _set_value(StoredType *pos, const T &t, _undirect_placement_layer) { **pos = t; }

    void _push_d(const DArrayData &d) {
        __push_d(d, __metatype());
    }
    void __push_d(const DArrayData &d, _direct_placement_layer) {
        if(__trivial_copy && __trivial_destruct) {
            if(_available() < d._size()) {
                __reserveUp(d._size() - _available());
            }
            copy_mem(data + size, d.data, d.size);
        } else {
            auto b = d.data;
            auto e = d.data + d.size;
            while(b!=e) {
                _push(*b++);
            }
        }
    }
    void __push_d(const DArrayData &d, _undirect_placement_layer) {
            auto b = d.data;
            auto e = d.data + d.size;
            while(b!=e) {
                _push(*(*b++));
            }
    }
    void _push(const T *src, int size) {
        if(__metatype::direct_placement && __trivial_copy && __trivial_destruct) {
            if(_available() < _size()) {
                __reserveUp(_size() - _available());
            }
            copy_mem(data + size, src, size);
        } else {
            auto b = src;
            auto e = src + size;
            while(b!=e) {
                _push(*b++);
            }
        }
    }
    void _push() { __push(__getPlace(), __metatype()); }
    void _push(const T &src) { __push(__getPlace(), src, __metatype()); }

    void __push(StoredType *place, _direct_placement_layer) {
        if(__trivial) {
            *place = T();
        } else {
            new (place) T();
        }
    }
    void __push(StoredType *place, const T &src, _direct_placement_layer) {
        if(__trivial_copy) {
            *place = src;
        } else {
            new (place) T(src);
        }
    }
    void __push(StoredType *place, _undirect_placement_layer) {
        *place = new T;
    }
    void __push(StoredType *place, const T &src, _undirect_placement_layer) {
        *place = new T(src);
    }


    void _destruct(int i, _direct_placement_layer) {
        if(!__trivial_destruct) {
            data[i].~T();
        }
    }
    void _destruct(int i, _undirect_placement_layer) {
        delete data[i];
    }
    void _destruct(int begin, int end, _direct_placement_layer) {
        if(!__trivial_destruct) {
            auto b = data + begin;
            auto e = data + end;
            while(b!=e) {(--e)->~T();}
        }
    }
    void _destruct(int begin, int end, _undirect_placement_layer) {
        auto b = data + begin;
        auto e = data + end;
        while(b!=e) {delete *(--e);}
    }
    void _destructAll(_direct_placement_layer) {
        if(!__trivial_destruct) {
            auto b = data;
            auto e = data + size;
            while(b!=e) {(--e)->~T();}
        }
    }
    void _destructAll(_undirect_placement_layer) {
        auto b = data;
        auto e = data + size;
        while(b!=e) {delete *(--e);}
    }

    void _dropAll() {
        _destructAll(__metatype());
        free_mem(data);
        data = nullptr;
        alloced = 0;
        size = 0;
        outterData = false;
    }
};
template <class T, DArrayMemManager memType = Auto>
class DArray {
public:
    friend class DReadArray<T, memType>;
    typedef DArrayData<T, memType> inner_t;
    DArray() { //+
        w.hold(new DArrayData<T, memType>);
    }
    explicit DArray(int n) {
        w.hold(new DArrayData<T, memType>);
        appendNs(n);
    }
    void clear() { //+
        *this = DArray();
    }

    void drop() {
        detach();
        w.data()->_drop();
    }
    using iterator = typename inner_t::iterator;
    using const_iterator = typename inner_t::const_iterator;

    // undirect
    // direct + full trivial
    // direct + complex + trivial copy
    // direct + full complex

    bool empty() const {return w.data()->_size() == 0;} //+, +
    int size() const { return w.data()->_size(); } //+, +
    int capacity() const {return w.data()->_capacity();} //+, +
    int available() const {return w.data()->_available();} //+, +

    iterator begin() { detach(); return w.data()->_begin(); } //+, +, +
    iterator end() { detach(); return w.data()->_end(); } //+, +, +
    iterator first() { detach(); return w.data()->_begin(); } //+
    iterator last() { detach(); return w.data()->_end() - 1; } //+

    const_iterator constBegin() const { return w.data()->_begin(); } //+
    const_iterator constEnd() const { return w.data()->_end(); } //+
    const_iterator constFirst() const { return w.data()->_begin(); } //+
    const_iterator constLast() const { return w.data()->_end() - 1; } //+

    T& operator[](int i) { detach(); return w.data()->_ref(i); } //+, +, +
    const T& operator[](int i) const { return w.data()->_ref(i); } //+, +, +

    T& front() {detach(); return w.data()->_ref(0);} //+
    T& back() {detach(); return w.data()->_ref(size() - 1);} //+
    const T& constFront() const {return w.data()->_ref(0);} //+
    const T& constBack() const {return w.data()->_ref(size() - 1);} //+

    int indexOf(const T &t) const { return w.data()->_index_of(t); } //+, +
    int indexOfWithin(const T &t, int from, int to) const { return w.data()->_index_of(t, from, to); } //+, +
    int count(const T &t) const { return w.data()->_count(t); } //+
    int countWithin(const T &t, int from, int to) const { return w.data()->_count(t, from, to); } //+
    bool contain(const T &t) const { return w.data()->_contain(t); } //+
    bool containWithin(const T &t, int from, int to) const {return w.data()->_contain(t, from, to);} //+



    void append() { //+, +, +
        detach();
        w.data()->_push();
    }
    void append(const T &t) { //+, +, +
        detach();
        w.data()->_push(t);
    }
    void append(const T &t, int n) { //+, +, +
        if(n > 0) {
            detach();
            while(n--) {
                w.data()->_push(t);
            }
        }
    }
    void appendNs(int n) { //+, +, +
        if(n > 0) {
            detach();
            while(n--) {
                w.data()->_push();
            }
        }
    }
    void appendLine(const DArray<T> &a) {
        detach();
        inner_t *d = w.data();
        d->_push_d(*a.w.data());
    }
    void appendLine(const T *src, int size) {
        detach();
        inner_t *d = w.data();
        d->_push(src, size);
    }

    void insert(int pos, const T &t) { //+, +
        detach();
        w.data()->_insert(pos, t);
    }
    void insertBefore(const T &t, const T &beforeThis) { //+, +
        int i = indexOf(beforeThis);
        if(i != -1) insert(i, t);
    }
    void fill(const T &t) { //+, +
        detach();
        auto b = begin();
        auto e = end();
        while(b!=e)  *b++ = t;
    }
#define RUN(X) \
    auto b = begin(); \
    auto e = end(); \
    while(b!=e) { X(*b++);}
#define IRUN(X) \
    auto b = begin(); \
    auto e = end(); \
    int i = 0; \
    while(b!=e) { X(*b++, i++);}
#define CRUN(X) \
    auto b = constBegin(); \
    auto e = constEnd(); \
    while(b!=e) { X(*b++);}
#define ICRUN(X) \
    auto b = constBegin(); \
    auto e = constEnd(); \
    int i = 0; \
    while(b!=e) { X(*b++, i++);}

    void runIn(void (*cb)(T&)) {RUN(cb);}
    void runIn(void (*cb)(T&, int)) {IRUN(cb);} //+, +
    void runOut(void (*cb)(const T&)) const {CRUN(cb);}
    void runOut(void (*cb)(const T&, int)) {ICRUN(cb);}

    void removeByIndex(int i) { //+, +
        detach();
        w.data()->_remove_by_index(i);
    }
    void remove(const T &t) { //+, +
        detach();
        w.data()->_remove(t);
    }
    void removeFirst() { //+, +
        detach();
        removeByIndex(0);
    }
    void removeLast() { //+, +
        detach();
        removeByIndex(size());
    }

    void push_back() {append();} //+, +, +
    void push_back(const T &t) {append(t);} //+, +, +
    void push_front(const T &t) {insert(0, t);} //+, +, +
    void pop_back() {removeLast();} //+, +, +
    void pop_front() {removeFirst();} //+, +, +

    void reform(int n) { //+, +, +
        int s = size();
        if(n > s) {
            appendNs(n - s);
        } else if(n < s) {
            cutEnd(s - n);
        }
    }
    void cut(int start, int end) { //+, +
        if(size() && start >= 0 && start < size() && end >= 0 && end < size()) {
            detach();
            if(start == end) {
                removeByIndex(start);
            } else {
                if(start > end)
                    std::swap(start, end);
                w.data()->_cut(start, end);
            }
        }
    }
    void cutBegin(int n) { //+, +
        if(n > size() || n < 0) return;
        if(n == 1) return removeFirst();
        w.data()->_cut(0, n);
    }
    void cutEnd(int n) { //+, +
        if(n > size() || n < 0) return;
        if(n == 1) return removeLast();
        w.data()->_cut(size() - n, size());
    }

    int reserve(int n) { //+, +
        detach();
        w.data()->_reserve(n);
        return capacity();
    }

    void swap(DArray &a) {w.swap(a.w);}
    void elementsSwap(int f, int s) { //+, +
        if(f < 0 || f >= size() || s < 0 || s >= size()) return;
        detach();
        w.data()->_swap(f, s);
    }
    void movePart(int dstPos, int srcPos, int n) { //+, +
        if(srcPos < 0 || dstPos < 0 || srcPos == dstPos || n <= 0 || srcPos > size() - n || dstPos > size() - n)
            return;
        detach();
        w.data()->_relocate(dstPos, srcPos, n);
    }
    void moveElement(int dstPos, int srcPos) { //+, +
        if(srcPos < 0 || dstPos < 0 || srcPos == dstPos || srcPos > size() || dstPos > size())
            return;
        detach();
        w.data()->_relocate(dstPos, srcPos, 1);
    }
    void moveElementToEnd(int pos) { //+, +
        if(pos < 0 || pos > size() || pos == size()-1)
            return;
        detach();
        w.data()->_relocate(size()-1, pos, 1);
    }
    void moveElementToBegin(int pos) { //+, +
        if(pos < 0 || pos > size() || pos == 0)
            return;
        detach();
        w.data()->_relocate(0, pos, 1);
    }

    void copyValue(const DArray &a) {
        detach();
        w.data()->_copy_data(*a.w.data());
    }
    void copyValue(const T *src, int size) {
        detach();
        w.data()->_copy_data(src, size);
    }
    void force(const DArray &a) {
        detach();
        w.data()->_force_data(*a.w.data());
    }
    void force(const T *src, int size) {
        detach();
        w.data()->_force_data(src, size);
    }
    void setSource(T *src, int size) {
        detach();
        w.data()->_set_source(src, size);
    }


    const DDualWatcher<inner_t>& watcher() const {return w;}


private:
    void detach() {
        w.detach();
//        std::cout << "call detach:"
//                  << " isu: " << w.isUnique()
//                  << " refs: " << w.base->refs()
//                  << std::endl;
    }
private:
    DDualWatcher<inner_t> w;
};
template <class T, DArrayMemManager memType = Auto>
class DReadArray {
public:
    DReadArray(const DArray<T, memType> &a) : base(a) {}
    DReadArray<T, memType>& operator=(const DArray<T, memType> &a) {base = a; return *this;}

    using const_iterator = typename DArray<T, memType>::const_iterator;

    bool empty() const {return base.empty();}
    int size() const { return base.size(); }
    int capacity() const {return base.capacity();}
    int available() const {return base.available();}

    const_iterator begin() const { return base.constBegin(); }
    const_iterator end() const { return base.constEnd(); }
    const_iterator first() const { return base.constFirst(); }
    const_iterator last() const { return base.constLast(); }

    const T& operator[](int i) const { return base[i]; }

    const T& front() const {return base.constFront();}
    const T& back() const {return base.constBack();}

    int indexOf(const T &t) const { return base.indexOf(t); }
    int indexOfWithin(const T &t, int from, int to) const { return base.indexOfWithin(t, from, to); }
    int count(const T &t) const { return base.count(t); }
    int countWithin(const T &t, int from, int to) const { return base.countWithin(t, from, to); }
    bool contain(const T &t) const { return base.contain(t); }
    bool containWithin(const T &t, int from, int to) const {return base.containWithin(t, from, to);}

    const DArray<T, memType>& inner() const {return base;}

    void setSource(const T *src, int size) {
        base.detach();
        base.w.data()->_set_const_source(src, size);
    }
private:
    DArray<T, memType> base;
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
