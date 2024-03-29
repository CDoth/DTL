#ifndef DBOX_H
#define DBOX_H
#include "DArray.h"

/*
template <class T>
class DBox
{
public:
    DBox() : current(0) {}
    explicit DBox(int n) : current(0)
    {
        store.reserve(n);
        while(n--) store.append();
    }
    DBox(int n, const T& t) : current(0)
    {
        store.reserve(n);
        while(n--) store.append(t);
    }
    void addItem(){store.append();}
    void addItem(const T&t){store.append(t);}

    typedef T* item_p;
    item_p pull()
    {
//        return current == store.size() ? nullptr : *(store.raw_begin() + current++);
    }
    int  push(item_p i)
    {
//        if(i)
//        {
//            auto b = store.raw_begin();
//            auto e = store.raw_begin() + current;
//            while(b!=e)
//            {
//                if(*b == i)
//                {
//                    std::swap(*b, *(store.raw_begin() + --current));
//                    break;
//                }
//                ++b;
//            }
//        }
        return available();
    }
    int size() const{return store.size();}
    int available() const{return store.size() - current;}
    int in_use() const{return current;}
    inline void clear()
    {
        store.clear();
        current = 0;
    }
    inline bool empty() const {return store.empty();}
    inline void remove(item_p i) {if(store.contain_within(i, current, store.size())) store.remove(i);}

    typedef typename DArray<T, Undirect>::iterator iterator;
    typedef typename DArray<T, Undirect>::const_iterator const_iterator;

    iterator begin(){return store.begin();}
    iterator end(){return store.end();}
    iterator av_begin(){return store.begin() + current;}
    iterator av_end(){return store.end();}
    iterator us_begin(){return store.begin();}
    iterator us_end(){return store.begin() + current;}
    const_iterator begin() const {return store.constBegin();}
    const_iterator end() const {return store.constEnd();}
    const_iterator av_begin() const {return store.constBegin() + current;}
    const_iterator av_end() const {return store.constEnd();}
    const_iterator us_begin() const {return store.constBegin();}
    const_iterator us_end() const {return store.constBegin() + current;}
private:
    int current;
    DArray<T, Undirect> store;
};
*/
/*
 test block
    DBox<A> box;

    box.addItem(11);
    box.addItem(22);
    box.addItem(33);

    auto h1 = box.pull();
    auto h2 = box.pull();
    auto h3 = box.pull();

    box.push(h3);

    printf("size: %d available: %d in use: %d \n", box.size(), box.available(), box.in_use());

    printf("Box:\n");
    auto b = box.begin();
    auto e = box.end();
    while(b!=e) printf("%d\n", (*b++).v);
    printf("Available:\n");
    b = box.av_begin();
    e = box.av_end();
    while(b!=e) printf("%d\n", (*b++).v);
    printf("In use:\n");
    b = box.us_begin();
    e = box.us_end();
    while(b!=e) printf("%d\n", (*b++).v);
 */

#endif // DBOX_H
