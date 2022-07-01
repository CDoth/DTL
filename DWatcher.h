#ifndef DWATCHER_H
#define DWATCHER_H

#include <iostream>

// 0. Create: set data to watcher
// 1. Copy: include object to watcher
// 2. Destroy: leave watcher
// 3. Destroy last object: leave and delete target data
// 4. Detach

#include <mutex>
#include <atomic>


class DBaseWatcher {
public:
    DBaseWatcher() : _refs(0), _data(nullptr) {
    }
    DBaseWatcher(void *data) : _refs(1), _data(data) {}
    void watchThis(void *d)  {
        _data = d;
        _refs = 1;
    }
    int pull() {
        if(!isUnique()) return leave();
        return _refs;
    }
    int leave() {return _refs > 0 ? --_refs : 0;}
    int addRef() {return ++_refs;}


    int refs() const {return _refs;}
    void *data() {return _data;}
    const void *data() const {return _data;}
    bool isUnique() const {return _refs == 1;}

private:
    int _refs;
    void *_data;
};
class DSafeThreadBaseWatcher {
public:
    DSafeThreadBaseWatcher() : _refs(0), _data(nullptr) {
    }
    DSafeThreadBaseWatcher(void *data) : _refs(1), _data(data) {
    }
    void watchThis(void *d)  {
        std::lock_guard<std::mutex> lock(__mutex);
        _data = d;
        _refs = 1;
    }
    int pull() {
        std::lock_guard<std::mutex> lock(__mutex);
        if(!isUnique()) return leave();
        return _refs;
    }
    int leave() {
        std::lock_guard<std::mutex> lock(__mutex);
        return _refs > 0 ? --_refs : 0;
    }
    int addRef() {
        std::lock_guard<std::mutex> lock(__mutex);
        return ++_refs;
    }
    int refs() const {
        std::lock_guard<std::mutex> lock(__mutex);
        return _refs;
    }
    void *data() {
        std::lock_guard<std::mutex> lock(__mutex);
        return _data;
    }
    const void *data() const {return _data;}
    bool isUnique() const {
        std::lock_guard<std::mutex> lock(__mutex);
        return _refs == 1;}

private:
    std::atomic<int> _refs;
    std::atomic<void*> _data;
    mutable std::mutex __mutex;
};


template <class DataType>
class DAbstractWatcher {

public:
//    typedef void DataType;

    DAbstractWatcher() : base(nullptr) {}
    ~DAbstractWatcher() {
    }
    void hold(DataType *data) {
        _pull();
        base = new DBaseWatcher(data);
    }
    void detach() {_detach();}
    void release() {
       _kickMe();
    }

    DAbstractWatcher(const DAbstractWatcher& a) : base(a.base) { _addMe(); }
    DAbstractWatcher(DAbstractWatcher& a) : base(a.base) { _addMe(); }
    DAbstractWatcher& operator=(const DAbstractWatcher& a) { _copy(a); return *this; }
    DAbstractWatcher& operator=(DAbstractWatcher& a) { _copy(a); return *this; }

    DAbstractWatcher(const DAbstractWatcher&& a) = delete;
    DAbstractWatcher(DAbstractWatcher&& a) = delete;
    DAbstractWatcher& operator=(const DAbstractWatcher&& a) = delete;
    DAbstractWatcher& operator=(DAbstractWatcher&& a) = delete;

    DataType* data() {return base ? reinterpret_cast<DataType*>(base->data()) : nullptr;}
    const DataType* data() const {return base ? reinterpret_cast<const DataType*>(base->data()) : nullptr;}
private:
    int _pull() {
        if(base) return base->pull();
        return 0;
    }
    void _detach() {
        if(base) {
            if(!base->isUnique()) {
                base->leave();
//                std::cout << "1. _detach: call cloneTarget" << std::endl;
                base = new DBaseWatcher(cloneTarget(data()));
//                std::cout << "1. called" << std::endl;
            }
        }
    }
    void _copy(const DAbstractWatcher &w) {
        if(base) {
            if( base->leave() == 0) {
//                std::cout << "2. _copy: call destroyTargetdata: " << (void*)base->data() << std::endl;
                destroyTarget();
//                std::cout << "2. called" << std::endl;
                delete base;
            }

        }
        base = w.base;
        base->addRef();
    }
    void _addMe() {
        if(base)
            base->addRef();
    }
    void _kickMe() {
        if(base) {
            if(base->leave() == 0) {
//                std::cout << "3. _kickMe: call destroyTarget: data: " << (void*)base->data() << std::endl;
                destroyTarget();
//                std::cout << "3. called" << std::endl;
                delete base;
                base = nullptr;
            }
        }
    }
private:
    DBaseWatcher *base;
protected:
    virtual DataType* cloneTarget(DataType*d) {
        if(d) return new DataType(*d);
    }
    virtual void destroyTarget() {
        auto d = data();
        if(d) delete d;
    }
};
#define BASE_CALL(INNERCALL, ERRORPROC) \
    if(data()) return data()->INNERCALL; \
    else ERRORPROC;


template <class DataType, class BaseWatcher = DBaseWatcher>
class DWatcher {

public:
    inline bool operator==(const DWatcher &other) const {return base ? (base == other.base) : false;}

    DWatcher(bool createTarget = false) : base(nullptr) {
        if(createTarget)
            base = new BaseWatcher(new DataType);
    }
    template <class ... Args>
    DWatcher(bool createTarget, Args && ...a) : base(nullptr) {
        if(createTarget)
            base = new BaseWatcher(new DataType(a...));
//        else
//            base = new DBaseWatcher;


//        std::cout << "  Create DWatcher(2): "
//                  << this
//                  << " base: " << base
//                  << std::endl;
    }
    // 0:
    DWatcher(DataType *data) {



        base = new BaseWatcher(data);

//        std::cout << "  Create DWatcher(3): "
//                  << this
//                  << " base: " << base
//                  << std::endl;
    }
    void hold(DataType *data) {
        if(base) {
            if(base->leave()) {
                base = new BaseWatcher;
            }
        } else {
            base = new BaseWatcher;
        }
        if(base) {
            base->watchThis(data);
        }
    }
    template <class ... Args>
    void createInner(Args && ...a) {
        if(base) {
            if(base->data() == nullptr) base->watchThis(new DataType(a...));
        } else {
            base = new BaseWatcher(new DataType(a...));
        }
    }
    // 1:
    DWatcher(const DWatcher& a) : base(a.base) {_addMe();}
    ///
    /// \brief operator =
    /// By default if you copy to DWatcher which already contain target data -
    /// object leave this watcher and if it were last watcher - data will be free
    /// with simple 'delete data()'
    /// If you want overload this behaivor read 'copy(const DWatcher &a)' brief
    ///
    /// \param a
    /// \return
    ///
    DWatcher& operator=(const DWatcher& a) {_copy(a); return *this;}
//    DWatcher& operator=(DWatcher& a) {_copy(a); return *this;}
    // 2:
    ~DWatcher() {
        _kickMe();
    }
    // 3:
    //automatically with desctructor
    // or:
    ///
    /// \brief release
    /// Call this in outer high-class destructor and check return value.
    /// You can ignore this function and leave outer destructor empty and let ~DWatcher automatically leave
    /// from watcher and free data with simple 'delete data()'
    /// \return
    /// nullptr: Object leaved watcher, but contained target data no need to free,
    /// because there is some watchers which hold target data.
    ///
    /// Pointer to DataType: Object leaved watcher and now user need to free returned data
    /// manually after call release()
    ///
    DataType* release() {
        return makeUnique();
    }
    void clearObject() {
        _kickMe();
    }
    // 4:
    ///
    /// \brief makeUnique
    /// Leave by watcher (without cloning data!)
    /// \return
    /// Pointer to DataType: Object leave watcher. User should
    /// clone returned data and set it to DWatcher object with 'hold()' function.
    ///
    /// nullptr: Object already is unique or no target data to watch. No need to clone data,
    /// just ignore this
    ///
    DataType* makeUnique() {
        if(base && !base->isUnique()) {
            auto d = data();
            base->leave();
            base = nullptr;
            return d;
        }
        return nullptr;
    }
    ///
    /// \brief detach
    /// Call makeUnique and clone target data with "new DataType(data())"
    /// \return
    ///
    void detach() {
        if(base) {
            auto d = makeUnique();
            if(d) {
                hold(new DataType(*d));
            }
        }
    }
    ///
    /// \brief copy
    /// Ignore this function if default copy behaivor fit you (read
    /// operator=() brief).
    ///
    /// To overload copy behaivor you need to overload copy operators
    /// (operator=) in user high-class and call there DWatcher::copy by your DWatcher object
    ///
    /// \param w
    /// \return
    /// nullptr: Object leaved watcher, but contained target data no need to free,
    /// because there is some watchers which hold target data
    ///
    /// Pointer to DataType: Object leaved watcher and now user need to free returned data
    /// manually after call copy()
    DataType* copy(const DWatcher &w) {
        DataType *d = nullptr;
        if(base) {
            if(base->leave() == 0) {
                d = data();
                delete base;
            }
        }
        base = w.base;
        base->addRef();

        return d;
    }
    void moveFrom( DWatcher &w ) {
        clearObject();
        base = w.base;
        w.base = nullptr;
    }
    void moveTo( DWatcher &w ) {
        w.clearObject();
        w.base = base;
        base = nullptr;
    }
    inline void defaultCopy(const DWatcher &w) {_copy(w);}
    bool isUnique() const {return base ? base->isUnique() : true;}
    DataType* data() {return base ? reinterpret_cast<DataType*>(base->data()) : nullptr;}
    const DataType* data() const {return base ? reinterpret_cast<const DataType*>(base->data()) : nullptr;}

    inline bool isEmptyObject() const {return base == nullptr;}
    inline bool isCreatedObject() const {return base;}

    void * raw_data() {return base ? base->data() : nullptr;}
    const void * raw_data() const {return base ? base->data() : nullptr;}
private:
    void _addMe() {
        if(base)
            base->addRef();
    }
    void _kickMe() {
        if(base) {
            if(base->leave() == 0) {
                auto d = data();
                if(d) {
                    delete d;
                }
                delete base;
            }
            base = nullptr;
        }
    }
    void _copy(const DWatcher &w) {
        if(base) {

            if( base->leave() == 0) {
                auto d = data();
                if(d) {
                    delete d;
                }
                delete base;
            }
        }
        base = w.base;
        if(base) {
            base->addRef();
        }
    }


private:
    BaseWatcher *base;
};
template <class DataType>
class DSafeThreadWatcher : public DWatcher<DataType, DSafeThreadBaseWatcher> {};



enum WatcherMode {ShareWatcher = 0, CloneWatcher = 1};
WatcherMode invertSide(WatcherMode m);


class DBaseDualWatcher {
public:
    DBaseDualWatcher() {
        _data = nullptr;
        _refs = 0;
        _mode = CloneWatcher;
        _otherSide = nullptr;
    }
    DBaseDualWatcher(WatcherMode mode) {
        _data = nullptr;
        _refs = 0;
        _mode = mode;
        _otherSide = nullptr;
    }
    DBaseDualWatcher(void *data, WatcherMode mode = CloneWatcher) {
        _data = data;
        _refs = 1;
        _mode = mode;
        _otherSide = nullptr;
    }
    void watchThis(void *data) {
        disconnect();
        _data = data;
        _refs = 1;
    }
    void setData(void *data) {
        _data = data;
    }

    DBaseDualWatcher* otherSide() {
        return _otherSide;
    }

    int leave() {return _refs > 0 ? --_refs : 0;}
    int include() {return ++_refs;}

    void* data() {return _data;}
    const void* data() const {return _data;}

    bool isUnique() const {return refs() == 1;}
    bool isUniqueInSide() const {return _refs == 1;}

    int refs() const {return _refs + (_otherSide ? _otherSide->_refs : 0);}
    int refsInSide() const {return _refs;}

    bool isClone() const {return _mode == CloneWatcher;}
    bool isShare() const {return _mode == ShareWatcher;}

    DBaseDualWatcher* disconnect() {
        DBaseDualWatcher *os = _otherSide;
        if(_otherSide) {
            _otherSide->_otherSide = nullptr;
        }
        _otherSide = nullptr;
        return os;
    }
private:
    DBaseDualWatcher* _getOtherSide() {
        if(_otherSide == nullptr) {
            _otherSide = new DBaseDualWatcher(invertSide(_mode));
            _otherSide->_otherSide = this;
        }
        return _otherSide;
    }

private:
    void *_data;
    int _refs;

    WatcherMode _mode;
    DBaseDualWatcher *_otherSide;
};

template <class DataType>
class DDualWatcher {
//    typedef void* DataType;
public:
    DDualWatcher() {
        base = new DBaseDualWatcher;
    }
    DDualWatcher(DataType *data) {
        base = new DBaseDualWatcher(data);
    }
    void hold(DataType *data) {
        if(base) {

        } else {
            base = new DBaseDualWatcher;
        }
        if(base) {
            base->watchThis(data);
        }
    }
    DDualWatcher(const DDualWatcher &a) : base(a.base) {_addMe();}
    DDualWatcher(DDualWatcher &a) : base(a.base) {_addMe();}
    DDualWatcher& operator=(const DDualWatcher &a) {_copy(a); return *this;}
    DDualWatcher& operator=(DDualWatcher &a) {_copy(a); return *this;}

    void swap(DDualWatcher &w) {std::swap(base, w.base);}

    ~DDualWatcher() {
        _kickMe();
    }

    DataType* release() {
        return makeUnique();
    }
    DataType* makeUnique() {
        if(base && !base->isUnique()) {
            auto d = data();
            base->leave();
            base = nullptr;
            return d;
        }
        return nullptr;
    }
    void detach() {
        if(base) {
            if(base->isClone()) {
                auto d = makeUnique();
                if(d) {
                    hold(new DataType(*d));
                }
            }
//            else if(base->isShare() && !base->isUnique()) {
//                auto d = data();
//                base->disconnect();
//                if(d) {
//                    base->setData(new DataType(*d));
//                }
//            }
        }
    }
    DataType* detach2() {
        if(base) {
            if(base->isClone()) {
                return makeUnique();
            } else if(base->isShare() && !base->isUnique()) {
                auto d = data();
                base->disconnect();
                if(d) {
                    return d;
                }
            }
        }
        return nullptr;
    }
    DataType* copy(const DDualWatcher &w) {
        DataType *d = nullptr;
        if(base) {
            if(base->leave() == 0) {
                d = data();
                delete base;
            }
        }
        base = w.base;
        base->include();
    }

    bool isUnique() const {return base ? base->isUnique() : true;}

    DataType* data() {return base ? reinterpret_cast<DataType*>(base->data()) : nullptr;}
    const DataType* data() const {return base ? reinterpret_cast<const DataType*>(base->data()) : nullptr;}
private:
    void _kickMe() {
        if(base) {
            if(base->leave() == 0) {
                auto d = data();
                if(d) {
                    delete d;
                }
                delete base;
            }
            base = nullptr;
        }
    }
    void _addMe() {
        if(base) {
            base->include();
        }
    }
    void _copy(const DDualWatcher &a) {
        if(base) {
            if(base->leave() == 0) {
                auto d = data();
                if(d) {
                    delete d;
                }
                delete base;
            }
        }
        base = a.base;
        if(base) {
            base->include();
        }
    }
public:
    DBaseDualWatcher *base;

};

class DDualWatcherStatic {
    typedef void* DataType;
public:
    DDualWatcherStatic() {
        base = new DBaseDualWatcher;
    }
    DDualWatcherStatic(DataType *data) {
        base = new DBaseDualWatcher(data);
    }
    void hold(DataType *data) {
        if(base) {
            if(base->isClone() && base->leave()) {
                base = new DBaseDualWatcher;
            }
        } else {
            base = new DBaseDualWatcher;
        }
        if(base) {
            base->setData(data);
        }
    }
    DDualWatcherStatic(const DDualWatcherStatic &a) : base(a.base) {_addMe();}
    DDualWatcherStatic(DDualWatcherStatic &a) : base(a.base) {_addMe();}

    ~DDualWatcherStatic() {
        _kickMe();
    }

    void* release() {
        return makeUnique();
    }
    void* makeUnique() {
        if(base && !base->isUnique()) {
            auto d = data();
            base->leave();
            base = nullptr;
            return d;
        }
        return nullptr;
    }
    void* detach() {
        if(base) {
            if(base->isClone()) {
                return makeUnique();
            } else if(base->isShare() && !base->isUnique()) {
                auto d = data();
                base->disconnect();
                if(d) {
                    return d;
                }
            }
        }
        return nullptr;
    }
    void* copy(const DDualWatcherStatic &w) {
        void *d = nullptr;
        if(base) {
            if(base->leave() == 0) {
                d = data();
                delete base;
            }
        }
        base = w.base;
        base->include();

        return d;
    }

    bool isUnique() const {return base ? base->isUnique() : true;}

//    DataType* data() {return base ? reinterpret_cast<DataType*>(base->data()) : nullptr;}
//    const DataType* data() const {return base ? reinterpret_cast<const DataType*>(base->data()) : nullptr;}
    void* data() {return base ? base->data() : nullptr;}
    const void* data() const {return base ? base->data() : nullptr;}
private:
    void _kickMe() {
        if(base) {
            if(base->leave() == 0) {
                delete base;
            }
            base = nullptr;
        }
    }
    void _addMe() {
        if(base) {
            base->include();
        }
    }
    void _copy(const DDualWatcherStatic &a) {
        if(base) {
            if(base->leave() == 0) {
                delete base;
            }
        }
        base = a.base;
        if(base) {
            base->include();
        }
    }
private:
    DBaseDualWatcher *base;
};







#endif // DWATCHER_H
