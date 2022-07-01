#ifndef DDIRREADER_H
#define DDIRREADER_H

#include <vector>
#include <string.h>
#include <dirent.h>
#include <fstream>

#include "DArray.h"


size_t get_file_size(const char* path);
size_t get_file_size(const std::string &path);
std::string get_short_size(size_t bytes);
std::string getBaseName(const std::string &name_with_extension);


///
/// \brief read_file
/// Alloced buffer and write all data from file to buffer. Caller should free buffer.
/// \param path
/// Path of file.
/// \param size
/// Size of data to read by file.
/// Set it to '0' or to negative value to ignore it and read all file.
/// \param error_code
/// Set error code to '*error_code':
/// -1: Bad (NULL) pointer to 'path'
/// -2: Wrong path (can't open file or zero file size)
/// -3: Fail of alloc memory for buffer
/// -4: Read '0' bytes
/// Set NULL to ingore this.
/// \param read
/// If read != NULL 'read_file' set to '*read' number of bytes read from file
/// \return
/// Pointer to buffer with file data or NULL if error occur (read 'error_code' description)
///
uint8_t* read_file(const char *path, size_t size, int *error_code, size_t *read); //alloc new memory, use free_file_data() (or free_mem()) fo free it
///
/// \brief free_file_data
/// Free buffer by 'get_file_data' function. Wrapper of free_mem().
/// \param buffer
/// Pointer to data
///
void free_file_data(uint8_t *buffer);

int path_correct(std::string &path, bool endSeparator = true, bool beginSeparator = false);
int path_correct_debug(std::string &path, bool endSeparator = true, bool beginSeparator = false);
std::string path_merge(const std::string &prefix, const std::string &suffix);
void path_attach(std::string &prefix, const std::string &suffix);
int path_cut_last_section(std::string &path);
int path_is_default(const std::string &path);
std::string path_get_last_section(const std::string &path);
void path_create_dir_tree(const std::string &path);
void path_create_dir_tree(const std::string &path, int cutSections);

void printTopology(const std::string &topology);

class DDirectory;
class DFileDescriptor;
class DDirReader;


struct DChangeItem;
class DChangeLog;


// Remove object:

// Exclude from parent list (innerDirs or files)
// Return key
// If dir: clear this



// 1. Exclude
// Exclude object from file system:
// - User should delete object after excluding to avoid memory leak
// - Directory object register FS_EXCLUDE_ACTION with object key


// 2. Move
// Exclude object from parent directory and push it to other file system (or to same file system but place it to another directory)
// - If object move to another file system object get new key
// - Moved object register FS_MOVE_ACTION

// 3. Remove
// Exclude object from parent directory and remove it at all
//====================================================

/*
struct __key_data {

    __key_data() : value(-1), parent(nullptr), isFree(true) {
//        std::cout << "++++++++++++ __key_data: create: " << this << std::endl;
    }
    ~__key_data() {
//        std::cout << "++++++++++++ __key_data: destruct: " << this << std::endl;
    }
    int value;
    DFileKeySystem *parent;
    bool isFree;
};
class DFileKey : public DWatcher<__key_data> {
public:
    friend class DFileKeySystem;
    friend class DFileDescriptor;
    DFileKey();
    explicit DFileKey(int value);
    ~DFileKey();

    inline bool operator==(int v) const {return data()->value == v;}
    inline bool operator==(const DFileKey &other) const { return data()->value == other.data()->value; }
    inline bool compare(const DFileKey &other) const {return other == *this;}
    inline bool operator<(const DFileKey &other) const {return value() < other.value();}
    inline bool operator>(const DFileKey &other) const {return value() > other.value();}

    const DFileKeySystem * parent() const;
    int value() const;
    bool isFree() const;
    bool returnMe();
private:
    explicit DFileKey(int value, DFileKeySystem *parent);
    void deinit();
    void free();
    void unfree();

};
class DFileKeySystem {
public:
    friend class DFileKey;
    DFileKeySystem();
    DFileKey getKey();
    bool returnKey(DFileKey &key);
    bool renew();
private:
    DArray<DFileKey> freeKeys;
    int nb_keys;
};
*/

typedef int DFileKey;
#define INVALID_DFILEKEY (-1)
#define BADKEY(KEY) (KEY < 0)
//====================================================



enum fs_action {

    FS_NO_ACTION

    ,FS_ADD_ACTION
    // DDirectory::addFile
    // DDirectory::addVirtualFile
    // DDirectory::addDirectory
    // DDirectory::addVirtualDirectory


    /// FS_EXCLUDE_ACTION register for excluded object(key)
    ,FS_EXCLUDE_ACTION
    // fs_object::exclude

    /// FS_MOVE_ACTION register for moved object(key)
    ,FS_MOVE_ACTION
    // fs_object::moveTo
    // DDirectory::acceptIt

    /// FS_REMOVE_ACTION register for removed object(key)
    ,FS_REMOVE_ACTION
    // DDirectory::removeFile
    // DDirectory::removeDirectory
    // delete object


    ,FS_META_ACTION
    ,FS_DATA_ACTION

};
const char * actionName(fs_action);

struct __fs_meta_data {
    __fs_meta_data() : __size(0) {}
    std::string __name;
    std::string __real_path;
    std::string __v_path;
    size_t __size;
};
struct fs_object {
public:
    friend class DDirectory;
    friend class DFileDescriptor;
    fs_object();
    virtual ~fs_object() {}
    void clearStats();
public:
    const std::string & getStdName() const {return meta.__name;}
    const std::string & getStdPath() const {return meta.__real_path;}
    const std::string & getStdVPath() const {return meta.__v_path;}

    const char* getName() const {return meta.__name.empty() ? nullptr : meta.__name.c_str();}
    const char* getPath() const {return meta.__real_path.empty() ? nullptr : meta.__real_path.c_str();}
    const char* getVPath() const {return meta.__v_path.empty() ? nullptr : meta.__v_path.c_str();}
    size_t getSize() const {return meta.__size;}
    __fs_meta_data getMeta() const {return meta;}
    std::string getShortSize() const {return sShortSize;}
    DDirectory* getParentDirectory() {return parent;}
    const DDirectory* getParentDirectory() const {return parent;}


    void setStatsByPath(const std::string &path);
    void createAbsolutePath(const std::string &pathWithoutName);
    void createVirtualPath(const std::string &realPrefix);
    void createVirtualPath(const std::string &realPrefix, const std::string &name);
    void setName(const std::string &name);
    void setPath(const std::string &path);


    virtual void moveTo(DDirectory * place) = 0;
    virtual fs_object* exclude() = 0;
    virtual void setSize(size_t s) = 0;

//    int key() const {return fs_key.value();}
    const DFileKey & key() const {return fs_key;}



private:
protected:
    std::string & sName() {return meta.__name;}
    const std::string & sName() const {return meta.__name;}

    std::string & sPath() {return meta.__real_path;}
    const std::string & sPath() const {return meta.__real_path;}

    std::string & sVPath() {return meta.__v_path;}
    const std::string & sVPath() const {return meta.__v_path;}

    size_t iSize() const {return meta.__size;}

    void __set_path(const std::string &p) {meta.__real_path = p;}
    void __set_name(const std::string &n) {meta.__name = n;}
    void __set_size(size_t size) {meta.__size = size;}
    
    void __set_parent(DDirectory *p);

protected:
    bool __create_vpath();
    void __clear_vpath();
    void action(fs_object *object, fs_action a);

    void acmute() {actionable = false;}
    void acunmute() {actionable = true;}
protected:

    __fs_meta_data meta;
    std::string sShortSize;
    DDirectory *parent;
    DFileKey fs_key;

protected:
    bool actionable;
    void (*action_callback)(void *base, fs_object *object, fs_action a);
    void *base;

    DChangeLog *changeLog;
};
//static inline void removeObject(fs_object *o) {if(o) delete o;}

struct DHistoryItem {
    DHistoryItem();
    fs_action __action;
    __fs_meta_data __meta_copy;
    fs_object *__actual_object;
};
struct DChangeItem {
    DChangeItem();
    fs_object *__object;
    fs_action __action;
};
class DChangeLog {
public:
    DChangeLog();
    void logIt(fs_object *o, fs_action a);
    void historyIt(fs_object *o, fs_action a);
    bool changeIt(fs_object *o, fs_action a);


    void setChangesMode(bool m) {do_changes = m;}
    void setHistoryMode(bool m) {do_history = m;}
    void clearHistory() {history.clear();}
    void clearChanges() {changes.clear();}
private:
    bool do_history;
    bool do_changes;
private:
    DArray<DHistoryItem> history;
    DArray<DChangeItem> changes;

};

typedef void*(*FileRegisterCallback)(DFileDescriptor*, void*);
typedef void (*FileExcludeCallback)(DFileDescriptor*, void*);


class DFileDescriptor : public fs_object {
public:
    DFileDescriptor();
    ~DFileDescriptor();

    DFileDescriptor(const DFileDescriptor &other);
public:
    int getLevel() const;
    void setSize(size_t size) override;
    void moveTo(DDirectory *place) override;
    fs_object* exclude() override;

private:
    friend class DDirectory;
    friend class DFileSystem;
    friend class DFile;

};
class DFile : public DWatcher<DFileDescriptor> {
public:
    DFile();
    DFile(const std::string &name, size_t size);
    explicit DFile(const std::string &path);
    ~DFile();

    void renew();
    bool initByPath(const std::string &path);
    void createAbsolutePath(const std::string &pathWithoutName);
    void createVirtualPath(const std::string &realPrefix);
    void createVirtualPath(const std::string &realPrefix, const std::string &name);

    inline operator bool() const {return bool(data());}
    DFileKey key() const;
    bool setParent(DDirectory *directory);

    void setKey(DFileKey value);

    //---------------------------------
    size_t size() const;

    std::string name() const;
    const char * name_c() const;

    std::string path() const;
    const char * path_c() const;

    std::string vpath() const;
    const char * vpath_c() const;

    std::string shortSize() const;
    const char *shortSize_c() const;
    const DDirectory * parent() const;

    DFileDescriptor * descriptor() {return data();}
    const DFileDescriptor * descriptor() const {return data();}
private:
    friend class DAbstractFileSystem;
};
#define BADFILE(_FILE_) (bool(_FILE_) == false)
DFileDescriptor * uniqueCopy(const DFileDescriptor *file);
DFileDescriptor * createVirtualFile(const std::string &name, size_t size = 0);
DFileDescriptor * createRegularFile(const std::string &path);

class DAbstractFileSystem;
class DDirectory : public fs_object {

public:
    bool open(const char *path);
    bool open();
    bool empty() const;

    //----------------------------------
    bool addFile(const char *path);
    DFile addVirtualFile(std::string name, size_t size = 0, DFileKey key = INVALID_DFILEKEY);
    DDirectory* addDirectory(const char *path);
    DDirectory* addVirtualDirectory(std::string name, size_t size = 0);
    //---------------------------------
    void renew();

    bool setFileSystem(DAbstractFileSystem *s);
    void removeFileSystem(bool propagateDown = true);

    void setSize(size_t size) override;
    void moveTo(DDirectory *place) override;
    fs_object* exclude() override;

    bool acceptIt(DFile &file);
    bool acceptIt(DDirectory *dir);

    bool removeFile(int index);
    bool removeFile(const std::string &name);
    bool removeDirectory(int index);
    bool removeDirectory(const std::string &name);

private:
    void __remove_object(fs_object *o);


public:

    DFile excludeFile(DFile &file);
    DDirectory * excludeDirectory(DDirectory * dir);


    static DFile shared_null;

public:

    std::string parseToText(bool includeFiles = true) const;

    int getFilesNumber() const;
    int getInnerDirsNumber() const;
    int getLevel() const;

    DFile getFile(int index);
    const DFile & getFile(int index) const;
    DFile & getFile(const std::string &name);
    const DFile & getConstFile(const std::string &name) const;

    bool containFile(const std::string &name) const;


    DDirectory* getDirectory(int index);
    const DDirectory* getDirectory(int index) const;
    DDirectory* getDirectory(const std::string &name);
    const DDirectory* getDirectory(const std::string &name) const;

public:
    typedef std::string::size_type topology_position_t;
    std::string topology(bool includeFiles) const;
    bool create(const std::string &topology);
    bool create(const std::string &topology, topology_position_t &pos);



    void clearDirList();
    void clearFileList();
    void clearLists();
    void clear();

public:
    DDirectory();
    ~DDirectory();
    friend class DDirReader;
private:
    bool __isVirtual();
    void __unreg();
    void __renew();

    void __inner_parse_to_text(std::string &text, bool includeFiles) const;
    bool __updateTopology(std::string &t, bool includeFiles) const;
    bool __create_lf_dirs(const std::string &topology, std::string::size_type &p);
    std::string __create_extract_name(const std::string &t, std::string::size_type &p);
    size_t __create_extract_size(const std::string &t, std::string::size_type &p);

    size_t __init_total_size();
    size_t __renew_total_size();
    bool __open();

    bool __add_objects();

    void __up_size(size_t s);
    void __down_size(size_t s);


    bool __add_regobject(std::string objectPath);
    bool __add_regfile(std::string filePath);
    DDirectory* __add_regdir(std::string dirPath);


private:
//    DArray<DFile> rootFiles;
//    DArray<DFile> alienFiles;

//    DArray<DDirectory*> rootDirectories;
//    DArray<DDirectory*> alienDirectories;

    DArray<DFile> files;
    DArray<DDirectory*> innerDirs;
private:

    DAbstractFileSystem *sys;

    bool bContentChanged;
    int iLevel;

};
std::string __extract_name(const std::string &t, std::string::size_type &p);
size_t __extract_size(const std::string &t, std::string::size_type &p);
int __extract_int(const std::string &t, std::string::size_type &p);
void __push_size(size_t size, std::string &to);
void __push_int(int value, std::string &to);
//=================================================================================================
//=================================================================================================
//=================================================================================================
class DAbstractFileSystem {
public:
    virtual void clear() = 0;
    virtual bool registerFile(DFile &file) = 0;
    virtual bool registerFile(DFile &file, int key) = 0;
    virtual bool unregFile(DFileKey key) = 0;
    virtual DFile file(DFileKey key) = 0;
    virtual const DFile & fileConstRef(DFileKey key) const = 0;
    virtual bool isKeyRegister(DFileKey key) const = 0;
    inline bool isKeyFree(DFileKey key) const {return !isKeyRegister(key);}
    virtual int size() const = 0;
public:
    static const DFile shared_null;
};
class DLinearFileSystem : public DAbstractFileSystem {
public:
    void clear() override;
    bool registerFile(DFile &file) override;
    bool registerFile(DFile &file, DFileKey key) override;
    bool unregFile(DFileKey key) override;
    DFile file(DFileKey key) override;
    const DFile & fileConstRef(DFileKey key) const override;
    bool isKeyRegister(DFileKey key) const override;
    int size() const override;

private:
    DArray<DFile> space;
    DArray<DFileKey> freeKeys;
private:
    bool __reg(DFile &file);
    bool __unreg(DFileKey key);
    bool __push(DFile &file, DFileKey key);
};
class DMapFileSystem : public DAbstractFileSystem {
public:
    void clear() override;
    bool registerFile(DFile &file) override;
    bool registerFile(DFile &file, DFileKey key) override;
    bool unregFile(DFileKey key) override;
    DFile file(DFileKey key) override;
    const DFile & fileConstRef(DFileKey key) const override;
    bool isKeyRegister(DFileKey key) const override;
    int size() const override;

private:
    std::map<int, DFile> space;
};




#endif // DDIRREADER_H
