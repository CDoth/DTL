#ifndef DLIST_H
#define DLIST_H
#include <DHolder.h>
#include <DItem.h>


template <class T>
class DListData
{
public:
    struct Item
    {
        Item() : next(nullptr), data(nullptr) {}
        ~Item() { if(data) delete data;}
        Item* next;
        T* data;
    };

    DListData() : start(nullptr), head(nullptr), size(0) {}
    void push_item(Item* item)
    {
        if(!start) start = item;
        if(head) head->next = item;
        head = item;
        ++size;
    }
    DListData(const DListData& l) : start(nullptr), size(0)
    {
        Item* out = l.start;
        while(out)
        {
            Item* i = new Item;
            i->data = new T(*out->data);
            push_item(i);
            out = out->next;
        }
    }
    ~DListData()
    {
        freeData();
    }
    void freeData()
    {
        Item* item = start;
        Item* next = nullptr;
        while(item)
        {
            next = item->next;
            delete item;
            item = next;
        }
    }
    Item* start;
    Item* head;
    int size;
};

template <class T>
class DList : public DHolder<DListData<T>>
{
public:
    DList();
    DList(int s);
    DList(int s, const T&);
    ~DList(){d = nullptr;}

    typedef typename DListData<T>::Item Item;

    struct iterator
    {
        iterator() : it(nullptr) {}
        iterator(Item* i) : it(i) {}

        inline iterator& operator++(){it = it->next; return *this;}
        inline iterator operator++(int){iterator ri = *this; it = it->next; return ri;}
        inline T& operator*(){return *it->data;}
        inline bool operator==(const iterator& i) const {return it==i.it;}
        inline bool operator!=(const iterator& i) const {return it!=i.it;}
        iterator& operator=(iterator i){it = i.it;return *this;}
    private:
        Item* it;
    };

    iterator begin(){return iterator(d->start);}
    iterator last(){return iterator(d->head);}
    iterator end(){return nullptr;}

    void add(const T& v);
    void add(T& v);
    void add();
    template <class ... Args>
    void add(Args...);
    void clear();
    int size();
private:
    DListData<T>* d;
};
template <class T>
DList<T>::DList() : DHolder<DListData<T> > (), d(this->alloc())
{
}
template <class T>
DList<T>::DList(int s) : DHolder<DListData<T> > (), d(this->alloc())
{
    while(s--) add();
}
template <class T>
DList<T>::DList(int s, const T&v) : DHolder<DListData<T> > (), d(this->alloc())
{
    while(s--) add(v);
}
template <class T>
void DList<T>::add()
{
    Item* item = new Item;
    item->data = new T;
    d->push_item(item);
}
template <class T>
void DList<T>::add(const T& v)
{
    Item* item = new Item;
    item->data = new T(v);
    d->push_item(item);
}
template <class T>
void DList<T>::add(T& v)
{
    Item* item = new Item;
    item->data = new T(v);
    d->push_item(item);
}
template <class T>
template <class ... Args>
void DList<T>::add(Args ... a)
{
    Item* item = new Item;
    item->data = new T(a...);
    d->push_item(item);
}
template <class T>
void DList<T>::clear()
{
    d->freeData();
}
#endif // DLIST_H
