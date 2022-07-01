#include "DWatcher.h"

/*
void DDualWatcher::disconnect(void *_d)
{
    if(_d) data = _d;
    if(other_side) other_side->other_side = nullptr;
//        if(other_side->refs() == 0) delete other_side;
    other_side = nullptr;
}
int DDualWatcher::pull()
{
    refDown();
    if(refs() == 0 && other_side) delete other_side;
    return refs();
}
DDualWatcher *DDualWatcher::otherSide()
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
*/

//123

