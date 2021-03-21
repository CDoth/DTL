#ifndef DBOX_H
#define DBOX_H
#include "DItem.h"
#include "DArray.h"
/*
template <class T, class TItem = DItems::MetaItem<T>>
class DBox
{
public:
    DBox();
    explicit DBox(int elems);
    DBox(const DBox&);
    ~DBox();
    void addItem();
    void addItem(const T& v);

    class Item : public TItem
    {
    public:
        friend class DBox;
        Item() : TItem(), strict_next(nullptr), original(nullptr) {}
        Item(const T&v) : TItem(v), strict_next(nullptr), original(nullptr) {}
        int self_push(){return reinterpret_cast<DBox<T,TItem>*>(d.parent)->push(this);}
        int operator()(){return self_push();}
        Item(const Item& item) : TItem(item){original = item.original; d.parent = item.d.parent;}
    private:
        DItems::item_data d;
        Item* strict_next;
        Item* original;
    };

    typedef Item* item_holder;
    Item* pull();
    int   push(Item*);
    const int& size() const {return _size;}
    const int& available() const {return _available;}
    void push_all();
private:

    void push_item(Item*);
    Item* start;
    Item* current;
    int _size;
    int _available;
};

template <class T, class I>
DBox<T,I>::DBox() : start(nullptr), current(nullptr), _size(0), _available(0)
{
}
template <class T, class I>
DBox<T,I>::DBox(int elems) : start(nullptr), current(nullptr), _size(0), _available(0)
{
    while(elems--) addItem();
}
template <class T, class I>
void DBox<T,I>::push_item(Item* i)
{
    i->d.parent = this;
    i->d.index = _size;
    i->original = i;
    if(current)
        i->d.next = current;
    current = i;
    if(start)
        i->strict_next = start;
    start = i;

    ++_size;
    ++_available;
}
template <class T, class I>
DBox<T,I>::DBox(const DBox& b)
{
    _size = b._size;
    _available = b._available;
    for(auto item = b.start; item; item = item->strict_next) push_item(new Item(*item));
}
template <class T, class I>
void DBox<T,I>::addItem()
{
    push_item(new Item);
}
template <class T, class I>
void DBox<T,I>::addItem(const T&v)
{
    push_item(new Item(v));
}
template <class T, class I>
typename DBox<T,I>::Item* DBox<T,I>::pull()
{
    Item* pull_it = current;
    if(current)
    {
        current = static_cast<Item*>(current->d.next);
        pull_it->d.next = nullptr;
        pull_it->d.is_shared = true;
        --_available;
    }
    return pull_it;
}
template <class T, class I>
int DBox<T, I>::push(Item* i)
{
    if(!i) return -1;
    i = i->original;
    if(i->d.parent != this) return -1;
    if(!i->d.is_shared) return -1;

    i->d.is_shared = false;
    i->d.next = current;
    current = i;
    return _size - ++_available;
}
template <class T, class I>
void DBox<T,I>::push_all()
{
    for(auto item = start; push(item); item = item->strict_next);
}
template <class T, class I>
DBox<T,I>::~DBox<T, I>()
{
    for(auto item = start; item; item = item->strict_next) delete item;
}
*/

template <class T, class TItem = DItems::MetaItem<T>>
class DBox
{
public:
    DBox() { current = 0; }
    explicit DBox(int n)
    {
        while(n--) store.append(TItem());
        current = 0;
    }
    DBox(int n, const T& t)
    {
        while(n--) store.append(TItem(t));
        current = 0;
    }

    void setValue(const T&t, int i)
    {
        store[i] = t;
    }
    void addItem()
    {
        store.append();
    }
    void addItem(const T&t)
    {
        store.append(t);
    }

    typedef TItem* item_p;
    item_p pull()
    {
        return current == store.size() ? nullptr : store.begin() + current++;
    }
    int  push(item_p i)
    {
        bool b = store.contain(*i);
//        if(i && store.contain(*i, 0, current))
//        {
//            std::swap(*i, *(store.begin() + --current));
//        }
        return available();
    }
    int size() const
    {
        return store.size();
    }
    int available() const
    {
        return store.size() - current;
    }



private:
    int current;
    DArray<TItem> store;
};


#endif // DBOX_H
