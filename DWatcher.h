#ifndef DWATCHER_H
#define DWATCHER_H

class DWatcher
{
public:
    DWatcher() {ref_count = new int(1);}
    bool is_unique() const {return *ref_count == 1;}
    void detach() {if(!is_unique()){--(*ref_count); ref_count = new int(1);}}
    int pull() {if(--(*ref_count) == 0) {delete ref_count; return 0;} else return *ref_count;}
    void refUp() {++(*ref_count);}
    int refs() const {return *ref_count;}
//private:
    int* ref_count;
};
enum WatcherMode {ShareWatcher = 1, CloneWatcher = 2};
struct DDualWatcher
{
    DDualWatcher(void* _data, WatcherMode _mode) : other_side(nullptr), data(_data), n(1), m(_mode) {}
    bool is_unique() const {return  refs() == 1;}
    //------------------------------
    int refUp() {return ++n;}
    int refDown() {return --n;}
    void disconnect()
    {
        if(other_side) other_side->other_side = nullptr;
        other_side = nullptr;
    }
    void disconnect(void* _d)
    {
        data = _d;
        if(other_side) other_side->other_side = nullptr;
        other_side = nullptr;
    }
    int pull()
    {
        refDown();
        if(refs() == 0 && other_side) delete other_side;
        return refs();
    }
    //------------------------------
    void* d() {return data;}
    const void* d() const {return data;}
    DDualWatcher* otherSide()
    {
        if(!other_side)
        {
            DDualWatcher* _other_side = new DDualWatcher(data, (m == ShareWatcher? CloneWatcher : ShareWatcher) );
            _other_side->n = 0;
            _other_side->other_side = this;
            other_side = _other_side;
        }
        return other_side;
    }
    int refs() const {return n + (other_side? other_side->n : 0);}
    int sideRefs() const {return n;}
    int otherSideRefs() const {return other_side? other_side->n : 0;}
    WatcherMode mode() const {return m;}
    bool is_clone() const {return m == CloneWatcher;}
    bool is_share() const {return m == ShareWatcher;}
    bool is_otherSide() const {return (bool)other_side;}
    //------------------------------
private:
    DDualWatcher* other_side;
    void* data;
    int n;
    WatcherMode m;
};
#endif // DWATCHER_H
