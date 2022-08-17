# DTL
Set of template classes which use in Doth projects and more.

( Working and using in others projects project but in my opinion code is young compared with lates projects. Actual project with my current code-style: DPeerNode. So projects with "young code" in plan to rework )

## Usable
Some tools in DTL in raw state and will ne reworked or removed or separate as single projects.
Usable tools list:
```
DArray
DWatcher
DDirReader
DLexeme
dmem
DProfiler
```
# DArray
DArray - simple container which I can modify for all my needs. 
### DArrayMemManager
```
enum DArrayMemManager {Direct, Undirect, Auto};
```
This template argument ignoring if DArray will use `DTrivialArrayData` as core (see `Meta behaivor` part).
For `DArrayData` core this mean how objects will be placed in memory.
> Direct

Direct way mean object will placed literally in DArray memory, like in C-style. But there is different placement for trivial and complex types.
- For trivial types with template argument `Direct` - usual C-style array
- For complex types with template argument `Direct` - usual C-style array, but objects created with `placement new` semantics.

> Undirect

Undirect way mean all objects, independently of type, will create with simple `new T` semantics and DArray will contain only pointers to objects.

> Auto

With this template argument DArray will choose between `Direct` and `Undirect` automatically by this algorihm:
```
    struct __metatype : std::conditional
    < (memType == Auto),
    // Auto mode:
        typename std::conditional

        < ( sizeof(T) > sizeof(void*) ),
            // Large types -> undirect
            _undirect_placement_layer,
            // Small types:
            typename std::conditional
            <__trivial_copy,
                // trivially copyable -> direct or cpx direct
                _direct_placement_layer,
                // no trivially copyable -> undirect
                _undirect_placement_layer
            >::type
        > ::type,

    // Specific modes:
        typename std::conditional
        <(memType == Direct),
            _direct_placement_layer, _undirect_placement_layer
        >::type
    > ::type
    {};
```

### Meta behaivor
DArray has some meta-behaivor for different types. 

DArray code starts with this code:
```
 typedef
    typename

    std::conditional<

        __INTEGRAL(T) || __FLOATING(T) || __POINTER(T),

        DTrivialArrayData<T>, DArrayData<T, memType>

    >::type
            __DATA_T;

    typedef DWatcher<__DATA_T> target_type;
```

Where:
```
T - target type of objects in container
__INTEGRAL(T) - check: is T integral type (int, char...)
__FLOATING(T) - check: is T floating type (float, double)
__POINTER(T)  - check: is T pointer to any type
DWatcher      - see DWatcher part
```
This mean if `std::conditional` condition will true - DArray will use `DTrivialArrayData<T>` as core. In alternative case `DArrayData<T, memType> will be core.

> As you can see if type is trivial - DArray will choose `DTrivialArrayData` as core and `DArrayData` in alternative case, 
but `DArrayData` can work with any type - even with trivial types. This happens because `DArrayData` was sole DArray core in past and this its original code.
Now for trivial types I wrote separeted `DTrivialArrayData` core but `DArrayData` code still in its previous state
### DArray methods

DArray has simple container methods like std::vector or QVector like:
```
    iterator begin();
    iterator end();
    iterator first();
    iterator last();
```
Or:
```
    void push_back();
    void push_back(const T &t);
    void push_front(const T &t);
    void pop_back();
    void pop_front();
```
And my own methods for convenience:
```
// Methods using binary search:
    int unique_insert_pos(const T &t) const;
    int unique_sorted_insert(const T &t);
    int indexOf_bin(const T &t) const;
    int indexOfWithin_bin(const T &t, int from, int to) const;
    bool contain_bin(const T &t) const;
    bool containWithin_bin(const T &t, int from, int to) const;
    
// Run callback trhough all array
    void runIn(T (*cb)()) ; 
    void runIn(void (*cb)(T&)) ;
    void runIn(void (*cb)(T&, int)) ;
    void runOut(void (*cb)(const T&)) const ;
    void runOut(void (*cb)(const T&, int)) const ;
    
// Remove methods:
    void cut(int start, int end); // remove objects from start to end
    void cutBegin(int n) ; // remove n objects from start
    void cutEnd(int n) ; // remove last n objects
    
// Exnand array by n elements and shift n objects to n positions
    void shiftPart(int from, int n) 
    
// Move part of array and shift another part between srcPos and dstPos
    void movePart(int dstPos, int srcPos, int n) 

// For trivial type `drop()` just set inner size variable to 0 without cleaning and without memory free. For complex types - objects will destructed.
  void drop();

// Check index
  bool inRange(int index) const;

// Fill value
  void fill(const T &t);
    
```
# DWatcher
> Smart copying

Suppose `class Descriptor` inherit by `DWatcher<YourData>` - so this class will behave as reference. If you copy incstance of this class to another instance - 
both instances will containt pointer to same data. So `const` methods of `Descriptor` will use same data to do something, but non constant methods
can call `DWatcher::detach()` before do changes to create unique copy of target data. You also can ignore `detach()` method in non-constant methods - in 
this case your `Descriptor` objects will behave as reference.

> Smart destruction

DWatcher use reference counter and inner data will desctuct only when last hight-level class instance destructed.

> Implement interface to your data

Suppose `class Interface` inherit by `DWatcher<YourData>` and `class ConstInterface` also inherit by `DWatcher<YourData>`.
Define different methods in `Interface` and `ConstInterface` to sepparate access to your data instead put all methods in one single class.

In my current coding conception I use 3 main ideas of data and its access:
- The data
- Data descriptor without any methods of interface (just empty class pointed to data)
- Data interfaces

Point of this model - you contain descriptors of data which has behaivor of reference (no use detach in non-const methods), but this descriptors are 
usless if you want read/write data. They has no any methods except methods which return you interface object.
This interface objects provide you methods you need to read/write data. Also main point of this model - descriptors note if you pick interface of data and 
can block next interface getting for other users/threads/methods while you keep and use your interface instance.

> Size of objects

All objects which contain its data through DWatcher has `sizeof(DWatcherChild) == sizeof(void*)` - size of pointer in your system.

#### Copy policy

Use copy policy to define copy behaivor of DWatcher<T> instances:
```
enum DWatcherCopyPolicy {
    WATCHER__COPYABLE,
    WATCHER__MOVABLE,
    WATCHER__UNIQUE
};
```
Copy policy make sense only for source objects ( if you copy A to B, copy policy of B does not metter ).

Policy `WATCHER__COPYABLE` define default model of copying. When you copy object with `WATCHER__COPYABLE` DWatcher up reference counter
and both objects point to same data.

Policy `WATCHER__MOVABLE` define move sematnic. When you copy object with `WATCHER__MOVABLE` this object become invalid/empty,
but second object now handle target data. Reference counter does not change. Don't use source object after this copying.

Polciy `WATCHER__UNIQUE` ingore any copy actions. Source object still handle its data and second object still in its state.
Reference counter does not change.

Policy `WATCHER__SPECIAL` (in plan) can define user own behaivor. For example it is equal `WATCHER__COPYABLE` but with limit of copy 
(maximum 3 reference).


#### Base of DWatcher

DWatcher has 2 template arguments. First - your target data which DWatcher will contain. Second - Base of DWatcher.

By default base is `DBaseWatcher`. This class implement data storing and reference counter. You can inherit by `DBaseWatcher` 
and add to your base what you need or create new class.

> Keep this in mind - base of DWatcher is copy-space of data, and DWatcher objects is members of this copy-space. When all members leave
copy-space - data become unwatched and uselss so last leaver destruct data and copy-space.

If you want redefine your custom base - your implementation HAVE TO provide this methods (with this signatures and names):
```
    void watchThis(void *d) ; // Hold new data
    int leave() ; // DWatcher call this when destructing, detaching or just leave copy-space for another reasons
    int addRef() ; // DWatcher connects to this copy space
    bool isUnique() const; // You are single member in copy-space
    int refs() const ; // Members in copy-space
    void *data() ; 
    const void *data() const ;
    void setCopyPolicy( DWatcherCopyPolicy cp ) ;
    DWatcherCopyPolicy copyPolicy() const ;
```

# DDirReader
There is functions and classes to work with files and directories metadatas. DDirReader in not about working with files and files data, 
only lists of metadata.

### What is important here

### Abstract file systems
File system here is list of files with some usefull methods.

Class `DAbstractFileSystem` provide interface to file system. You can inherit from `DAbstractFileSystem` but better to inherit from
`DLinearFileSystem` and `DMapFileSystem`.

Both systems provide to register files to get unique keys for each file. `DLinearFileSystem` need for register raw file and generate new key for this file.
`DMapFileSystem` need to register file with your own key.

In pair both classes can be used like this:
- `DLinearFileSystem` generate source file system with own key-space
- `DMapFileSystem` trying copy part (or whole) of source file system
Another words it using to create sync mechanism for file catalogs where is `DLinearFileSystem` using on local side and `DMapFileSystem` using on
remote side to store copy of source file catalog on local side.

### DDirectory

This class provide to store file-directory tree. `DDirectory` store tree of another directories (also `DDirectory`) and files in this directories.
There is two ways to add file or dorectory to DDirectory:
- Read it from computer using `bool open(const char *path)` or `bool addFile(const char *path)` or `addDirectory(const char *path)`
- Add virtual notes of files and directories which unnecessary exist. For this way call 

`DFile addVirtualFile(std::string name, size_t size = 0, DFileKey key = INVALID_DFILEKEY)` 

or 

`DDirectory* addVirtualDirectory(std::string name, size_t size = 0)`

> For print or see on your tree use:

```
  std::string parseToText(bool includeFiles = true) const;
```

> To save your directory tree or transmit it to another machine use:

```
  std::string topology(bool includeFiles) const;
```
This method generate human-unreadable topolgy of your tree. 

> To restore directory tree by topology use:
```
  bool create(const std::string &topology);
```
This method create full virtual tree by topology.

### DFile

DFile inherit by DWatcher and you can use DFile instance like sharable no-const reference.
DFile store file metadata and pointer to parent DDirectory.
Usefull methods:

```
void renew(); // If real path of file set - check file on computer and update metadate such as file size
bool initByPath(const std::string &path); // Check file by this path and try fill its metadata
void createAbsolutePath(const std::string &pathWithoutName); // Merge `pathWithoutName` and file name and push result to `metadata::path`
void createVirtualPath(const std::string &realPrefix); 
```

# DLexeme
This is my mini regular expressions core.

DLexeme provide searching lexemes and expressions by:
- Simple rules by DLexeme methods such as `DArray<int> numbers()` - find all numbers in text or `lexeme base()` - find separeted lexeme by user limits
- User complex rules

### User rules
Set your rule with call method `DLexeme::set_rule`.
Example of rule to search IP address:
```
DLexeme::raw rule_address = "<.:3><$min: 7><$max: 15><$permsym: dig, .><$spec: \n>";
```
> Each subrule should be contained in `< >`
Example of IP address: `255.255.255.255`
`<.:3>` - mean desired lexeme should contain three dots 
`<$min: 7><$max: 15>` - mean desired lexeme should has lenght in range [7;15] symbols
`<$permsym: dig, .>` - mean in desired lexeme permitted only digits and dots
`<$spec: \n>`        - mean sybmols separators for lexemes in our text is `space` and `newline` symbol

# dmem
Low level global functions for using heap mem

## DProfiler
Simple namespace with timing functions



