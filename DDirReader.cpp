#include "DDirReader.h"
#include <algorithm>
#include "dmem.h"
#include <sys/stat.h>

#include <DLogs.h>

size_t get_file_size(const char* path)
{
    std::ifstream f(path, std::ios::in | std::ios::binary);
    if(f.is_open())
    {
        f.seekg(0, std::ios::end);
        return f.tellg();
    }
    else return 0;
}
size_t get_file_size(const std::string &path)
{
    return get_file_size(path.c_str());
}

#define GET_NAME(X) \
    case X: return "["#X"]"; break
const char* actionName(fs_action a) {
    switch (a) {
        default: return "[UNDEFINED ACTION]"; break;
        GET_NAME(FS_NO_ACTION);
        GET_NAME(FS_ADD_ACTION);
        GET_NAME(FS_DATA_ACTION);
        GET_NAME(FS_META_ACTION);
        GET_NAME(FS_REMOVE_ACTION);
    }
}




DLogs::DLogsContext log_context;
DLogs::DLogsContextInitializator logsInit("DDirReader", log_context);



void printTopology(const std::string &topology) {

    std::cout << "[";
    for(std::string::size_type i = 0; i != topology.size(); ++i) std::cout << topology[i];
    std::cout << "]";
    std::cout << std::endl;


    std::cout << std::endl;
}


std::string  get_short_size(size_t bytes) {
    if(bytes == 0) {
        return std::string("0 bytes");
    }
    static char buffer[30];
    double v = 0.0;
    for(;;) {
        int kb = bytes / 1024;  bytes %= 1024; if(!kb){v = bytes; snprintf(buffer, 30, "%.2f bytes", v);  break;}
        int mb = kb / 1024;    kb %= 1024;    if(!mb){v = kb + (double)bytes/1024; snprintf(buffer, 30, "%.2f Kb", v);  break;}
        int gb = mb / 1024;    mb %= 1024;    if(!gb){v = mb + (double)kb/1024; snprintf(buffer, 30, "%.2f Mb", v);  break;}
        int tb = gb / 1024;    gb %= 1024;    if(!tb){v = gb + (double)mb/1024; snprintf(buffer, 30, "%.2f Gb", v); break;}
        v = tb + (double)gb/1024; snprintf(buffer, 30, "%.2f Tb", v);   break;
    }
    return buffer;
}
std::string getBaseName(const std::string &name_with_extension)
{
    auto name = name_with_extension;
    auto b = name.begin();
    auto e = name.end();
    int pos = 0;
    while(b != e)
    {
        if(*b++ == '.')
        {
            return name.substr(0, pos);
        }
        ++pos;
    }
    return name;
}
uint8_t *read_file(const char *path, size_t size, int *error_code, size_t *read)
{
#define WRITE_ERROR(code) \
    if(error_code) *error_code = code;

    if(path == NULL)
    {
        WRITE_ERROR(-1);
        return NULL;
    }
    FILE *f = fopen(path, "rb");
    if(f == NULL)
    {
        WRITE_ERROR(-2);
        return NULL;
    }
    size_t file_size = get_file_size(path);
    if(file_size == 0)
    {
        WRITE_ERROR(-2);
        return NULL;
    }

    if(size > 0 && size < file_size)
        file_size = size;

    uint8_t *buffer = get_mem<uint8_t>(file_size);
    if(buffer == NULL)
    {
        WRITE_ERROR(-3);
        return NULL;
    }
    size_t r = fread(buffer, 1, file_size, f);
    if(read == 0 || r < file_size)
    {
        WRITE_ERROR(-4);
        free_mem(buffer);
        return NULL;
    }
    if(read) *read = r;
    fclose(f);
    return buffer;

#undef WRITE_ERROR
}
void free_file_data(uint8_t *buffer)
{
    free_mem(buffer);
}
int path_correct(std::string &path, bool endSeparator, bool beginSeparator) {

    // 1. remove duplicate separators
    // 2. only right separators
    // 3. separator on the end and the begin


    bool last_symb_separator = false;


    if(path.front() == '/') {
        if(!beginSeparator) path.erase(path.begin());
    } else {
        if(beginSeparator) path.insert(0, "/");
    }

    for(size_t i = 0; i != path.size(); ++i) {

        if(last_symb_separator) {
            if(path[i] == '\\' || path[i] == '/') {
                path.erase(i--, 1);
            } else {
                last_symb_separator = false;
            }
        } else if(path[i] == '\\' || path[i] == '/') {
            path[i] = '/';
            last_symb_separator = true;
        } else {
            last_symb_separator = false;
        }
    }

    if(path.back() == '/') {
        if(!endSeparator) path.erase(path.end()-1);
    } else {
        if(endSeparator) path.push_back('/');
    }

    return path.size();
}
int path_correct_debug(std::string &path, bool endSeparator, bool beginSeparator) {
    bool last_symb_separator = false;


    DL_INFO(1, "path: [%s]", path.c_str());


    for(size_t i = 0; i != path.size(); ++i) {

        if(last_symb_separator) {
            if(path[i] == '\\' || path[i] == '/') {
                path.erase(i--, 1);
            } else {
                last_symb_separator = false;
            }
        } else if(path[i] == '\\' || path[i] == '/') {
            path[i] = '/';
            last_symb_separator = true;
        } else {
            last_symb_separator = false;
        }
    }

    if(path.back() == '/') {
        if(!endSeparator) path.erase(path.end()-1);
    } else {
        if(endSeparator) path.push_back('/');
    }


    return path.size();
}
std::string path_merge(const std::string &prefix, const std::string &suffix) {

    if(prefix.empty()) {
        return suffix;
    }
    if(suffix.empty()) {
        return prefix;
    }

    std::string r = prefix;
    path_correct(r, true);
    r += suffix;
    path_correct(r, false);

    return r;
}
void path_attach(std::string &prefix, const std::string &suffix) {
    path_correct(prefix, true);
    prefix += suffix;
    path_correct(prefix, true);
}
int path_cut_last_section(std::string &path)
{
    std::string::iterator iter = path.end();
    std::string::iterator b = path.begin();
    std::string::iterator e = path.end();
    while(b != e)
    {
        if(*b == '/' || *b == '\\')
            iter = b;
        ++b;
    }
    path.erase(iter+1, e);
    return path.size();
}
int path_is_default(const std::string &path)
{
    std::string::const_iterator b = path.cbegin();
    std::string::const_iterator e = path.cend();
    while(b != e)
    {
        if(*b == '/' || *b == '\\') return 1;
        ++b;
    }
    return 0;
}
std::string path_get_last_section(const std::string &path) {
    std::string::const_iterator iter = path.end();
    std::string::const_iterator prev = path.end();
    std::string::const_iterator b = path.begin();
    std::string::const_iterator e = path.end();
    while(b != e) {
        prev = iter;
        if(*b == '/' || *b == '\\')
            iter = b;
        ++b;

    }

    if(path.back() == '/' || path.back() == '\\')
        return std::string(prev+1, e-1);
    else
        return std::string(iter+1, e);
}

void __inner_create_dir_tree(const std::string &p) {


    std::cout << "__inner_create_dir_tree source: "
              << p
              << std::endl;

    auto b = p.begin();
    auto e = p.end();
    while(b != e) {
        if(*b == '/') {
            std::string __lp =
                    std::string(p.begin(), b);

            int status = mkdir(__lp.c_str());

            std::cout << "MKDIR Source: "
                      << __lp
                      << " status: " << status
                      << std::endl;

        }
        ++b;
    }
}
void path_create_dir_tree(const std::string &path) {

    std::string p = path;

    path_correct(p, false);

//    DL_INFO(1, "p: [%s]", p.c_str());

    __inner_create_dir_tree(p);
}
void path_create_dir_tree(const std::string &path, int cutSections) {
    if(cutSections <= 0) {
        return path_create_dir_tree(path);
    }
    std::string p = path;

    path_correct(p, false);

    auto e = p.end();
    auto b = p.begin();
    int sections = 0;
    while(b != e) {
        --e;
        if(*e == '/') {
            if(++sections == cutSections) {
                p.erase(e, p.end());
                break;
            }
        }
    }

    std::cout << "CUTED PATH: "
              << p
              << std::endl;

    __inner_create_dir_tree(p);
}

//============================================================================================================================= DFileKey
/*
DFileKey::DFileKey() {

//    DL_INFO(1, "Create empty DFileKey: obj: [%p]", this);
}
DFileKey::DFileKey(int value) : DWatcher<__key_data>(true) {

    data()->value = value;

//    DL_INFO(1, "Create value DFileKey: obj: [%p] inner: [%p] value: [%d]", this, data(), value);
}
DFileKey::~DFileKey() {
//    DL_INFO(1, "Destruct DFileKey: obj: [%p]", this);
}
DFileKey::DFileKey(int value, DFileKeySystem *parent) : DWatcher<__key_data>(true) {

    data()->value = value;
    data()->parent = parent;
    data()->isFree = false;

//    DL_INFO(1, "Create value&parent DFileKey: obj: [%p] inner: [%p] value: [%d]", this, data(), value);
}

const DFileKeySystem *DFileKey::parent() const {
    if(data())
        return data()->parent;
//    else DL_BADPOINTER(1, "Base data");
    return nullptr;
}
int DFileKey::value() const {
    if(data()) {
        return data()->value;
    } else {
//        DL_BADPOINTER(1, "Base data");
        return -1;
    }
}
bool DFileKey::isFree() const {
    if(data()) {
        return data()->isFree;
    } else {
//        DL_BADPOINTER(1, "Base data");
        return false;
    }
}
bool DFileKey::returnMe() {
    if(data() && data()->parent) {
        return data()->parent->returnKey(*this);
    }
    return false;
}


void DFileKey::deinit() {
    if(data()) {
        data()->value = -1;
        data()->isFree = true;
        data()->parent = nullptr;
    } else {
        DL_BADPOINTER(1, "Base data");
    }
}
void DFileKey::free() {
    if(data()) {
        data()->isFree = true;        
    }
}
void DFileKey::unfree() {
    if(data()) {
        data()->isFree = false;
    }
}
*/
//============================================================================================================================= DFileKeySystem
/*
DFileKeySystem::DFileKeySystem() {
    nb_keys = 0;
}
DFileKey DFileKeySystem::getKey() {

//    DL_INFO(1, "freeKeys: [%d] nb_keys: [%d]", freeKeys.size(), nb_keys);

    if(freeKeys.empty()) {
        return DFileKey(nb_keys++, this);
    } else {
        DFileKey key = freeKeys.front();
        freeKeys.pop_front();
        key.unfree();
        return key;
    }
}
bool DFileKeySystem::returnKey(DFileKey &key) {

    if(key.value() < 0) {
        DL_ERROR(1, "Invalid key");
        return false;
    }
    if(key.parent() != this) {
        DL_ERROR(1, "Alien key");
        return false;
    }
    if(key.isFree()) {
        DL_ERROR(1, "Key already returned");
        return false;
    }


    if(key.value() == nb_keys - 1) {
        key.deinit();
        --nb_keys;
    }
    else {
        key.free();
        freeKeys.push_back(key);
    }

    renew();

    return true;
}
bool DFileKeySystem::renew() {

    // If all keys returned and no one in usage - renew system by clear list
    // of returned keys to let DFileSystem start again

    if(freeKeys.size() == nb_keys) {
        FOR_VALUE(freeKeys.size(), i) {
            freeKeys[i].deinit();
        }
        freeKeys.clear();
        nb_keys = 0;
        return true;
    }
    return false;
}
*/

//============================================================================================================================= fs_object
fs_object::fs_object() {
    parent = nullptr;
    base = nullptr;
    changeLog = nullptr;
    actionable = false;
    fs_key = INVALID_DFILEKEY;
}
void fs_object::clearStats() {
    sName().clear();
    sPath().clear();
    __set_size(0);
    sShortSize.clear();
    action(this, FS_DATA_ACTION);
}
void fs_object::setStatsByPath(const std::string &path) {
    if( path != meta.__real_path ) {
        setPath(path);
        setName(path_get_last_section(path));
    }
    __create_vpath();
    setSize(get_file_size(path));
}
void fs_object::createAbsolutePath(const std::string &pathWithoutName) {
    std::string prefix = pathWithoutName;
    path_correct(prefix);
    setPath(
                path_merge(prefix, meta.__name)
                );
    __create_vpath();
}
void fs_object::createVirtualPath(const std::string &realPrefix) {

    setPath(
                path_merge(realPrefix, meta.__v_path)
                );
}
void fs_object::createVirtualPath(const std::string &realPrefix, const std::string &name) {

//    std::string __v = path_
}
void fs_object::setName(const std::string &name) {
    meta.__name = name;
    __create_vpath();
//    action(FS_META, this);
}
void fs_object::setPath(const std::string &path) {
    meta.__real_path = path;
//    action(FS_META, this);
}
void fs_object::__set_parent(DDirectory *p) {
    if(p != parent) {
        parent = p;
        if(p) __create_vpath();
        else __clear_vpath();
    }
}
bool fs_object::__create_vpath() {


//    std::cout << "__create_vpath:"
//              << " object: " << (void*)this
//              << " name: " << sName()
//              << " parent: " << (void*)parent
//              << " parent name: " << parent->sName()
//              << " parent vpath: " << parent->sVPath()
//              << std::endl;

    if(sName().empty()) {
        DL_ERROR(1, "Empty name");
        return false;
    }
    if(parent) {

        if(parent->sVPath().empty()) {
            DL_ERROR(1, "Empty parent vpath. obj: [%p]", (void*)this);
            return false;
        }
        sVPath() = parent->sVPath();

        sVPath().append("/");
        sVPath().append(sName());

    } else {
        sVPath() = "/";
        sVPath().append(sName());
    }

    return true;
}
void fs_object::__clear_vpath() {
    meta.__v_path.clear();
}
void fs_object::action(fs_object *object, fs_action a) {

    if(!object || !object->actionable) return;
    if(action_callback) {
        action_callback(base, object, a);
    }
    if(changeLog) {
        changeLog->logIt(object, a);
    }
}
//============================================================================================================================= DHistoryItem
DHistoryItem::DHistoryItem() {

    __action = FS_NO_ACTION;
    __actual_object = nullptr;
}
//============================================================================================================================= DChangeItem
DChangeItem::DChangeItem() {
    __action = FS_NO_ACTION;
    __object = nullptr;
}
//============================================================================================================================= DChangeLog
DChangeLog::DChangeLog() {
    do_history = false;
    do_changes = false;
}
void DChangeLog::logIt(fs_object *o, fs_action a) {
    historyIt(o, a);
    changeIt(o, a);
}
void DChangeLog::historyIt(fs_object *o, fs_action a) {
    if(do_history && o) {
        DHistoryItem item;
        item.__action = a;
        item.__actual_object = o;
        item.__meta_copy = o->getMeta();
        history.push_back(item);
    }
}
bool DChangeLog::changeIt(fs_object *o, fs_action a) {
    if(do_changes && o) {
        FOR_VALUE(changes.size(), i) {
            auto __o = changes[i].__object;
            auto __a = changes[i].__action;
            if(__o == o) {
                if(a == FS_ADD_ACTION) {
                    DL_ERROR(1, "Object already exist (current action: FS_ADD_ACTION)");
                    return false;
                }
                changes[i].__action = fs_action(__a | a);
                return true;
            }
        }
        DChangeItem item;
        item.__action = a;
        item.__object = o;
        changes.push_back(item);
        return true;
    }
    return false;
}
//============================================================================================================================= DFileDescriptor
DFileDescriptor::DFileDescriptor() {
//    DL_INFO(1, "Create DFileDescriptor: [%p]", this);

}
DFileDescriptor::~DFileDescriptor() {
//    exclude();
//    key().returnMe();
//    action(this, FS_REMOVE_ACTION);

//    DL_INFO(1, "Destruct DFileDescriptor: [%p]", this);
}
DFileDescriptor::DFileDescriptor(const DFileDescriptor &other) {
    meta = other.meta;
    sShortSize = other.sShortSize;
    parent = nullptr;
    __clear_vpath();
}
int DFileDescriptor::getLevel() const {
    return parent ? parent->getLevel() + 1 : 0;
}
void DFileDescriptor::setSize(size_t size) {
    meta.__size = size;
    sShortSize = get_short_size(size);
    action(this, FS_META_ACTION);
}
void DFileDescriptor::moveTo(DDirectory *place) {
    if(place) {
//        place->acceptIt(this);
    }
}
fs_object *DFileDescriptor::exclude() {
    if(parent) {
//        return parent->excludeFile(this);
    }
    return nullptr;
}

DFileDescriptor *uniqueCopy(const DFileDescriptor *file) {

    return file ? new DFileDescriptor(*file) : nullptr;
}
DFileDescriptor *createVirtualFile(const std::string &name, size_t size) {


    DFileDescriptor *file = new DFileDescriptor;
    if(file) {
        file->setName(name);
        file->setSize(size);
    }

    return file;
}
DFileDescriptor *createRegularFile(const std::string &path) {

    DFileDescriptor *file = new DFileDescriptor;
    if(file) {
        file->setStatsByPath(path);
    }
    return file;
}
//============================================================================================================================= Directory
DFile DDirectory::shared_null;


DDirectory::DDirectory() {
    iLevel = 0;
    sys = nullptr;
}
DDirectory::~DDirectory() {
    clearLists();

}
bool DDirectory::__isVirtual() {
    return sPath().empty();
}
void DDirectory::__unreg() {
    if( sys == nullptr ) return;

    FOR_VALUE( innerDirs.size(), i ) {
        innerDirs[i]->__unreg();
    }
    FOR_VALUE( files.size(), i ) {
        sys->unregFile(files[i].key());
    }
}
void DDirectory::__renew() {

    if( __isVirtual() ) {

        FOR_VALUE( innerDirs.size(), i ) {
            innerDirs[i]->__renew();
        }

        int r = 0;
        struct stat s;
        FOR_VALUE( files.size(), i ) {
            if( (r = stat(files[i].path_c(), &s)) ) {
                files.removeByIndex(i--);
                continue;
            }
            if( s.st_mode & S_IFREG ) {
                files[i].renew();
                if( sys ) sys->registerFile(files[i]);
            } else {
                files.removeByIndex(i--);
            }

        }

    } else {
        open();
    }
}

void DDirectory::__inner_parse_to_text(std::string &text, bool includeFiles) const {


    if(iLevel == 0) {


        text.append("DDirectory: ");
        text.append(sName());

        // root directiory short size:
        text.append(" [");
        text.append(getShortSize());
        text.append("]");

        // root directiory inner dirs:
        text.append(" [d: ");
        text.append(std::to_string(innerDirs.size()));
        text.append("]");

        // root directiory inner files:
        text.append(" [f: ");
        text.append(std::to_string(files.size()));
        text.append("]");

        text.append("\n");
    }
    FOR_VALUE(innerDirs.size(), i) {
        auto d = innerDirs[i];
        text.append(iLevel + 1, ' ');
        text.append(">");


//         directiory name:
        text.append(d->getStdName());

        // directiory short size:
        text.append(" [");
        text.append(d->getShortSize());
        text.append("]");

        // directiory inner dirs:
        text.append(" [d: ");
        text.append(std::to_string(d->innerDirs.size()));
        text.append("]");

        // directiory inner files:
        text.append(" [f: ");
        text.append(std::to_string(d->files.size()));
        text.append("]");

        // directiory level:
        text.append("(L:");
        text.append(std::to_string(d->iLevel));
        text.append(")");

        text.append("\n");
        d->__inner_parse_to_text(text, includeFiles);

    }
    if(includeFiles) {
        FOR_VALUE(files.size(), i) {
            const DFile &file = files[i];
            text.append(iLevel + 1, ' ');
            text.append(file.name());

            // file short size:
            text.append(" [");
            text.append(file.shortSize());
            text.append("]");

            if(file.parent()) {
                text.append("(L:");
                text.append(std::to_string(file.parent()->iLevel));
                text.append(")");
            }

            text.append("[K:");
            text.append(std::to_string(file.key()));
            text.append("]");

            text.append("\n");
        }
    }


}

//-----------------------------------------------------------------
bool DDirectory::open(const char *path) {

    if(path == nullptr) {
        DL_BADPOINTER(1, "path");
        return false;
    }

    __set_path(path);
    return __open();
}
bool DDirectory::open() {
    return __open();
}
bool DDirectory::empty() const {
    return files.empty() && innerDirs.empty();
}
//============================================================================================
bool DDirectory::addFile(const char *path) {

    if( !__isVirtual() ) {
        DL_WARNING(1, "Can't add objects to regular directory. Make it virtual");
        return false;
    }

    if(path == nullptr) {
        DL_BADVALUE(1, "path");
        return shared_null;
    }

    struct stat s;
    int r = 0;
    if( (r = stat(path, &s)) ) {
        DL_FUNCFAIL(1, "stat(): [%d]", r);
        return shared_null;
    }
    if( !(s.st_mode & S_IFREG) ) {
        DL_ERROR(1, "[%s] - This is no file", path);
        return shared_null;
    }


    if( __add_regfile(path) == false ) {
        DL_FUNCFAIL(1, "__add_regfile");
        return false;
    }


    return true;
}
DFile DDirectory::addVirtualFile(std::string name, size_t size, DFileKey key) {

    if( !__isVirtual() ) {
        DL_WARNING(1, "Can't add objects to regular directory. Make it virtual");
        return DFile();
    }

    if(name.empty()) {
        DL_BADVALUE(1, "Empty name");
        return DFile();
    }
    if( getFile(name) ) {
        DL_ERROR(1, "File with name [%s] already exist", name.c_str());
        return DFile();
    }

    DFile file(name, size);
    file.setParent(this);

    if(sys) {
        if( sys->registerFile(file, key) == false ) {
            DL_ERROR(1, "Can't register file with key [%d]", key);
        }
    }

    __up_size(size);
    files.push_back(file);

    return file;
}
DDirectory *DDirectory::addDirectory(const char *path) {

    if( !__isVirtual() ) {
        DL_WARNING(1, "Can't add objects to regular directory. Make it virtual");
        return nullptr;
    }

    if(path == nullptr) {
        DL_BADVALUE(1, "path");
        return nullptr;
    }
    struct stat s;
    if( stat(path, &s) == 0) {
        if(s.st_mode & S_IFDIR) {
            DDirectory *d = __add_regdir(path);
            if(d == nullptr) {
                DL_FUNCFAIL(1, "__add_regdir");
                return nullptr;
            }

            __up_size(d->iSize());

//            action(d, FS_ADD_ACTION);
            return d;
        } else {
            DL_ERROR(1, "[%s] - This is no directory", path);
            return nullptr;
        }
    } else {
        DL_FUNCFAIL(1, "stat()");
        return nullptr;
    }
}
DDirectory *DDirectory::addVirtualDirectory(std::string name, size_t size) {


    if( !__isVirtual() ) {
        DL_WARNING(1, "Can't add objects to regular directory. Make it virtual");
        return nullptr;
    }

    if(name.empty()) {
        DL_BADVALUE(1, "name");
        return nullptr;
    }

    FOR_VALUE(innerDirs.size(), i) {

        if(innerDirs[i]->sName() == name) {
            DL_ERROR(1, "There is already exist directory with name [%s]", name.c_str());
            return nullptr;
        }
    }



    DDirectory *dir = new DDirectory;


    dir->__set_name(name);
    dir->sPath().clear();
    dir->parent = this;
    dir->__create_vpath();


    dir->setSize(size);
    dir->sShortSize = get_short_size(0);

    dir->iLevel = iLevel + 1;


    innerDirs.push_back(dir);

    dir->setFileSystem(sys);

    action(dir, FS_ADD_ACTION);

    return dir;
}
//============================================================================================
void DDirectory::renew() {
    
    __unreg();
    __renew();
    __renew_total_size();
    
}

bool DDirectory::setFileSystem(DAbstractFileSystem *s) {


    if(sys == nullptr
            ||
            (innerDirs.empty() && files.empty()) ) {
        sys = s;

//        DL_INFO(1, "Set system [%p] to directory: [%p]", sys, this);
        return true;
    }

    DL_ERROR(1, "Can't replace file system. First: remove existing file sysytem");
    return false;
}
void DDirectory::moveTo(DDirectory *place) {
    if(place) {
        place->acceptIt(this);
    }
}
fs_object* DDirectory::exclude() {
    if(parent) {
        return parent->excludeDirectory(this);
    }
    return nullptr;
}
bool DDirectory::acceptIt(DFile &file) {
    if(file && file.parent() != this) {

//        file->acmute();

//        file->exclude();


//        file->__set_parent(this);

//        files.push_back(file);

//        file->acunmute();


//        action(file, FS_MOVE_ACTION);

        return true;
    }

    return false;
}
bool DDirectory::acceptIt(DDirectory *dir) {

    if(dir && dir->getParentDirectory() != this) {

        dir->acmute();

        dir->exclude();


        dir->__set_parent(this);
        innerDirs.push_back(dir);

        dir->acunmute();


        action(dir, FS_MOVE_ACTION);

        return true;
    }

    return false;
}
bool DDirectory::removeFile(int index) {
//    __remove_object(
//                getFile(index)
//                );

    return true;
}
bool DDirectory::removeFile(const std::string &name) {
//    __remove_object(
//                getFile(name)
//                );

    return true;
}
bool DDirectory::removeDirectory(int index) {

    __remove_object(
                getDirectory(index)
                );

            return true;
}
bool DDirectory::removeDirectory(const std::string &name) {

    __remove_object(
                getDirectory(name)
                );

            return true;
}
void DDirectory::__remove_object(fs_object *o) {
    if(o) {
        delete o;
    }
}
DFile DDirectory::excludeFile(DFile &file) {

    if(file) {
        int p = files.indexOf(file);
        if(p < 0) {
            return shared_null;
        } else {
            files.removeByIndex(p);
            file.setParent(nullptr);

//            action(file, FS_EXCLUDE_ACTION);
            return file;
        }
    }
    return shared_null;
}
DDirectory *DDirectory::excludeDirectory(DDirectory *dir) {

    if(dir) {
        int p = innerDirs.indexOf(dir);
        if(p < 0) {
            return nullptr;
        } else {
            innerDirs.removeByIndex(p);

            dir->__set_parent(nullptr);

            action(dir, FS_EXCLUDE_ACTION);
            return dir;
        }
    }
    return nullptr;
}
void DDirectory::setSize(size_t size) {
    if(innerDirs.empty() && files.empty()) {
        meta.__size = size;
        action(this, FS_META_ACTION);
    }
}
std::string DDirectory::parseToText(bool includeFiles) const {
    std::string text;
    __inner_parse_to_text(text, includeFiles);
    return text;
}
//-----------------------------------------------------------------
bool DDirectory::__add_regobject(std::string objectPath) {

    struct stat s;
    int r = 0;



    if( (r = stat(objectPath.c_str(), &s)) == 0) {
        if( s.st_mode & S_IFDIR) {
            if( !__add_regdir(objectPath) ) {
                DL_FUNCFAIL(1, "addInnerDirectory");
                return false;
            }
        } else if (s.st_mode & S_IFREG) {

            if( __add_regfile(objectPath) == false ) {
                DL_FUNCFAIL(1, "addFile: [%s]", objectPath.c_str());
                return false;
            }

        } else {
            DL_ERROR(1, "Undefined object type");
            return false;
        }
    } else {
        DL_FUNCFAIL(1, "stat(): [%d] path: [%s] error: [%d]", r, objectPath.c_str(), errno);
        return false;
    }


    return true;
}
DDirectory *DDirectory::__add_regdir(std::string dirPath) {



    if(dirPath.empty()) {
        DL_BADVALUE(1, "dirPath");
        return nullptr;
    }

    std::string name = path_get_last_section(dirPath);

//    std::cout << "__add_regdir:"
//              << " path: " << dirPath
//              << " name: " << name
//              << " parent: " << (void*)this
//              << " parent name: " << this->sName()
//              << " sys: " << sys
//              << std::endl;


    FOR_VALUE(innerDirs.size(), i) {
        if(innerDirs[i]->sName() == name) {
            DL_ERROR(1, "There is already exist directory with name [%s]", name.c_str());
            return nullptr;
        }
    }

    DDirectory *dir = new DDirectory;




    dir->__set_name(name);
    dir->__set_path(dirPath);
    dir->parent = this;
    dir->__create_vpath();
    dir->setFileSystem(sys);

    if( dir->__open() == false ) {
        DL_FUNCFAIL(1, "__open");
//        delete dir;
//        return nullptr;
    }



    innerDirs.push_back(dir);


    return dir;
}

bool DDirectory::__add_regfile(std::string filePath) {

    std::string name = path_get_last_section(filePath);


    if(containFile(name)) {
        DL_ERROR(1, "File with name [%s] already exist", name.c_str());
        return shared_null;
    }

    DFile file(filePath);

    file.setParent(this);
    files.push_back(file);


    if(sys) {
        if( sys->registerFile(file) == false ) {
            DL_ERROR(1, "Can't register file");
        }
    }

    __up_size(file.size());


    return true;
}
bool DDirectory::__open() {


    if(sPath().empty()) {
        DL_ERROR(1, "Empty path");
        return false;
    }



    clearLists();
    path_correct(sPath());
    __set_name(path_get_last_section(sPath()));


    __create_vpath();
    iLevel = parent ? parent->iLevel + 1 : 0;


    if( __add_objects() == false ) {
        DL_FUNCFAIL(1, "__add_objects");
        return false;
    }
    __init_total_size();


    action(this, FS_DATA_ACTION);




    return true;
}
bool DDirectory::__add_objects() {

    DIR *DIRECTORY = nullptr;
    struct dirent *ent = nullptr;

    DIRECTORY = opendir(sPath().c_str());
    if(DIRECTORY == nullptr) {
        DL_FUNCFAIL(1, "opendir with path: [%s]", sPath().c_str());
        return false;
    }
    //=======================
    while( (ent = readdir(DIRECTORY)) ) {
        if( !(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) ) {
            std::string objectPath = sPath() + std::string(ent->d_name);
            __add_regobject(objectPath);
        }
    }

    return true;
}
void DDirectory::__up_size(size_t s) {

    meta.__size += s;
    sShortSize = get_short_size(iSize());

    DDirectory *np = parent;
    while(np) {
        np->__init_total_size();
        np = np->parent;
    }
}
void DDirectory::__down_size(size_t s) {

    if(iSize() < s) {
        __renew_total_size();
    } else {
        meta.__size -= s;
        sShortSize = get_short_size(iSize());
    }

    DDirectory *np = parent;
    while(np) {
        np->__init_total_size();
        np = np->parent;
    }
}
size_t DDirectory::__init_total_size() {

    __set_size(0);
    FOR_VALUE(innerDirs.size(), i) {
        meta.__size += innerDirs[i]->iSize();
    }
    FOR_VALUE(files.size(), i) {
        meta.__size += files[i].size();
    }
    sShortSize = get_short_size(iSize());
    return iSize();

        return 0;
}
size_t DDirectory::__renew_total_size() {

    __set_size(0);
    FOR_VALUE(innerDirs.size(), i) {
        meta.__size += innerDirs[i]->__renew_total_size();
    }
    FOR_VALUE(files.size(), i) {
        meta.__size += files[i].size();
    }
    sShortSize = get_short_size(iSize());
    return iSize();

        return 0;
}
//-----------------------------------------------------------------
int DDirectory::getFilesNumber() const {

    return files.size();

            return 0;
}
int DDirectory::getInnerDirsNumber() const {

    return innerDirs.size();

            return 0;
}
int DDirectory::getLevel() const {
    return iLevel;
            return 0;
}
DFile DDirectory::getFile(int index) {
    if(index >= 0 && index < files.size())
        return files[index];
    return shared_null;
}
const DFile & DDirectory::getFile(int index) const {
    if(index >= 0 && index < files.size())
        return files[index];
    return shared_null;
}
DFile & DDirectory::getFile(const std::string &name) {

    FOR_VALUE(files.size(), i ) {
        if(files[i].name() == name) {
            return files[i];
        }
    }
    return shared_null;
}
const DFile &DDirectory::getConstFile(const std::string &name) const {
    FOR_VALUE(files.size(), i ) {
        if(files[i].name() == name) {
            return files[i];
        }
    }
    return shared_null;
}
bool DDirectory::containFile(const std::string &name) const {
    FOR_VALUE(files.size(), i ) {
        if(files[i].name() == name) return true;
    }
    return false;
}
DDirectory *DDirectory::getDirectory(int index) {
    if(index >= 0 && index < innerDirs.size()) {
        return innerDirs[index];
    }
    return nullptr;
}
const DDirectory *DDirectory::getDirectory(int index) const {
    if(index >= 0 && index < innerDirs.size()) {
        return innerDirs[index];
    }
    return nullptr;
}
DDirectory *DDirectory::getDirectory(const std::string &name) {

    FOR_VALUE(innerDirs.size(), i) {
        if(innerDirs[i]->getStdName() == name) {
            return innerDirs[i];
        }
    }
    return nullptr;
}
const DDirectory *DDirectory::getDirectory(const std::string &name) const {

    FOR_VALUE(innerDirs.size(), i) {
        if(innerDirs[i]->getStdName() == name) {
            return innerDirs[i];
        }
    }
    return nullptr;
}
std::string DDirectory::topology(bool includeFiles) const {

    std::string t;
    t.append("!R");
    t.append(sName());
    t.append("/");

    if( !__updateTopology(t, includeFiles) ) {
        return std::string();
    }

    t.append("!O");

    return t;
}
bool DDirectory::create(const std::string &t) {
    if(innerDirs.size()) {
        DL_ERROR(1, "Directory already has topology, clear it before use create()");
        return false;
    }

    std::string::size_type pos = 0;
    if( !__create_lf_dirs(t, pos) ) {
        DL_FUNCFAIL(1, "Main call: __create_lf_dirs");
        return false;
    }

    return true;
}
bool DDirectory::create(const std::string &topology, DDirectory::topology_position_t &pos) {

    if(innerDirs.size()) {
        DL_ERROR(1, "Directory already has topology, clear it before use create()");
        return false;
    }





    if( !__create_lf_dirs(topology, pos) ) {
        DL_FUNCFAIL(1, "Main call: __create_lf_dirs");
        return false;
    }


    return true;
}

void DDirectory::clearDirList() {

    if(innerDirs.empty()) return;
//    FOR_VALUE(innerDirs.size(), i) {
//        auto dir = innerDirs[i];
//        if(dir) {

//            __down_size(dir->iSize());
//            dir->acmute();
//            dir->__set_parent(nullptr);
//            delete dir;
//        }
//    }
//    action(this, FS_DATA_ACTION);
    innerDirs.clear();
}
void DDirectory::clearFileList() {
    if(files.empty()) return;

    FOR_VALUE(files.size(), i) {
        DFile &f = files[i];

        if( BADFILE(f) ) {
            continue;
        }

        __down_size(f.size());
        f.setParent(nullptr);

//        if( sys ) {
//            DL_INFO(1, "unreg file [%d]", f.key());
//            sys->unregFile(f.key());
//        }
    }
    action(this, FS_DATA_ACTION);
    files.clear();
}
void DDirectory::clearLists() {

    clearDirList();
    clearFileList();
}
void DDirectory::clear() {

    clearLists();
    clearStats();
}
//-----------------------------------------------------------------
bool DDirectory::__create_lf_dirs(const std::string &t, std::string::size_type &p) {

    p = t.find('!', p);
    std::string::size_type term = -1;
    DDirectory *currentDirectory = nullptr;



    while(p != term ) {

        char s = t[p + 1];
        p += 2; //Skip marker
        if(s == 'D') {
            std::string dirName = __create_extract_name(t, p);
            if(dirName.empty()) {
                DL_FUNCFAIL(1, "__create_extract_name (dir name)");
                return false;
            }
//            size_t size = __extract_size(t, p);


            if( (currentDirectory = addVirtualDirectory(dirName)) == nullptr) {
                DL_FUNCFAIL(1, "v_addVirtualDirectory");
                return false;
            }
        } else if(s == 'F') {
            std::string fileName = __create_extract_name(t, p);
            if(fileName.empty()) {
                DL_FUNCFAIL(1, "__create_extract_name (file name)");
                return false;
            }
            size_t size = __extract_size(t, p);
            int key = __extract_int(t, p);

            if( !addVirtualFile(fileName, size, key) ) {
                DL_FUNCFAIL(1, "Inner call: innerAddVirtualFile2");
                return false;
            }

        } else if (s == 'O') {


//            std::cout << ">>>>>>>>>>>>>>> MARKER: [" << 'O' << "]"
//                      << " dir: [" << (void*)this << "]"
//                      << " pos: [" << p << "]"
//                      << std::endl;

            return true;

        } else if (s == 'I') {

            if(currentDirectory == nullptr) {
                currentDirectory = this;
            }

            if( !currentDirectory->__create_lf_dirs(t, p) ) {
                DL_FUNCFAIL(1, "Inner call: __create_lf_dirs");
                return false;
            }
//            currentDirectory = nullptr;

        } else if (s == 'R') {



            __set_name(__create_extract_name(t, p));
            if(sName().empty()) {
                DL_FUNCFAIL(1, "__create_extract_name (root name)");
                return false;
            }
            __create_vpath();




        } else {



            DL_ERROR(1, "Bad topology: undefined marker: [%c] pos: [%d]", s, p);
            return false;
        }
        p = t.find('!', p);
    }

    return true;
}
std::string DDirectory::__create_extract_name(const std::string &t, std::string::size_type &p) {

    return __extract_name(t, p);
}
size_t DDirectory::__create_extract_size(const std::string &t, std::string::size_type &p) {
    return __extract_size(t, p);
}
bool DDirectory::__updateTopology(std::string &t, bool includeFiles) const {


    if(innerDirs.empty() && (!includeFiles || files.empty()) ) {
        return true;
    }

    if(innerDirs.size() || (includeFiles && files.size())) {
        t.append("!I");
    }
    FOR_VALUE(innerDirs.size(), i) {
        DDirectory *d = innerDirs[i];

        t.append("!D");
        t.append(d->getName());
        t.append("/");

        __push_size(d->meta.__size, t);


        d->__updateTopology(t, includeFiles);
    }
    if(includeFiles) {
        FOR_VALUE(files.size(), i) {

            const DFile &file = files[i];

            t.append("!F");
            t.append(file.name());
            t.append("/");

            __push_size(file.size(), t);
            __push_int(file.key(), t);



        }
    }
    if(innerDirs.size() || (includeFiles && files.size())) {
        t.append("!O");
    }


    return true;

}
//-----------------------------------------------------------------



//========================================================================================================================== DDirReader
std::string __extract_name(const std::string &t, std::string::size_type &p) {
    auto epos = t.find('/', p);
    if(epos == p + 1) {
        return std::string();
    }
    if(epos != std::string::size_type(-1)) {
        std::string::const_iterator b = t.begin() + p;
        std::string::const_iterator e = t.begin() + epos;
        std::string dirName(b, e);

        p = epos + 1;
        return dirName;
    } else {
        DL_ERROR(1, "Bad topology: no [/] as dir name end pos: [%d]", p);
        return std::string();
    }
}
size_t __extract_size(const std::string &t, std::string::size_type &p) {

    size_t size = *reinterpret_cast<const size_t*>( t.data() + p );
    p += sizeof(size_t);
    return size;
}
int __extract_int(const std::string &t, std::string::size_type &p) {

    int v = *reinterpret_cast<const int*>( t.data() + p );
    p += sizeof(int);
    return v;
}
void __push_size(size_t size, std::string &to) {

    to.append(
                reinterpret_cast<const char*>(&size),
                ( sizeof (size) )
            );
}
void __push_int(int value, std::string &to) {
    to.append(
                reinterpret_cast<const char*>(&value),
                ( sizeof (value) )
                );
}


//=============================================================================== DFile
DFile::DFile() {
}
DFile::DFile(const std::string &name, size_t size) : DWatcher<DFileDescriptor>(true) {

    data()->__set_name(name);
    data()->setSize(size);
}
DFile::DFile(const std::string &path) {
    if(path.size()) {
        createInner();
        initByPath(path);
    }
}
DFile::~DFile() {
}
void DFile::renew() {
    initByPath(path());
}
bool DFile::initByPath(const std::string &path) {
    if( !isEmptyObject() && !path.empty() ) {

        data()->setStatsByPath(path);
        return true;
    }
    return false;
}
void DFile::createAbsolutePath(const std::string &pathWithoutName) {
    if(!isEmptyObject()) {
        data()->createAbsolutePath(pathWithoutName);
    }
}
void DFile::createVirtualPath(const std::string &realPrefix) {
    if(!isEmptyObject()) {
        data()->createVirtualPath(realPrefix);
    }
}
void DFile::createVirtualPath(const std::string &realPrefix, const std::string &name) {
    if(!isEmptyObject()) {
        data()->createVirtualPath(realPrefix);
    }
}
DFileKey DFile::key() const {

    if(isEmptyObject()) return -1;

    return data()->key();
}
bool DFile::setParent(DDirectory *directory) {

    if(isEmptyObject()) return false;

    if(data()->parent == nullptr)
        data()->__set_parent(directory);

    return true;
}
void DFile::setKey(DFileKey value) {

    if(!isEmptyObject()) {
        data()->fs_key = value;
    }
}
size_t DFile::size() const {

    if(isEmptyObject()) return 0;

    return data()->getSize();
}
std::string DFile::name() const {

    if(isEmptyObject()) return std::string();

    return data()->getStdName();
}
const char *DFile::name_c() const {

    if(isEmptyObject()) return nullptr;

    return data()->getName();
}
std::string DFile::path() const {
    if(isEmptyObject()) return std::string();

    return data()->getStdPath();
}
const char *DFile::path_c() const {

    if(isEmptyObject()) return nullptr;

    return data()->getPath();
}
std::string DFile::vpath() const {
    if(isEmptyObject()) return nullptr;

    return data()->getStdVPath();
}
const char *DFile::vpath_c() const {
    if(isEmptyObject()) return nullptr;

    return data()->getVPath();
}
std::string DFile::shortSize() const {
    if(isEmptyObject()) return std::string();

    return data()->getShortSize();
}
const char *DFile::shortSize_c() const {

    if(isEmptyObject()) return nullptr;

    return data()->getShortSize().c_str();
}
const DDirectory *DFile::parent() const {

    if(isEmptyObject()) return nullptr;

    return data()->parent;
}
//=============================================================================== DAbstractFileSystem
const DFile DAbstractFileSystem::shared_null;
//=============================================================================== DLinearFileSystem
bool DLinearFileSystem::__reg(DFile &file) {

    if(freeKeys.empty()) {
        file.setKey(space.size());
        space.push_back(file);
    } else {
        DFileKey key = freeKeys.back();
        if(key < 0 || key >= space.size()) {
            DL_BADVALUE(1, "free key: [%d]", key);
            return false;
        }
        if(space[key].isEmptyObject() == false) {
            DL_ERROR(1, "Uncleared file [%p] with free key: [%d]", space[key].descriptor(), key);
            return false;
        }
        file.setKey(key);
        space[key] = file;
        freeKeys.pop_back();
    }

    return true;
}
bool DLinearFileSystem::__unreg(DFileKey key) {

    int size = space.size();

    if(key < 0 || key >= size) {
        DL_BADVALUE(1, "key: [%d] size: [%d]", key, size);
        return false;
    }
    if(key == size - 1) {
        space.back().setKey(INVALID_DFILEKEY);
        space.back().clearObject();
        space.pop_back();

        if(freeKeys.size() == space.size()) {
            freeKeys.clear();
            space.clear();
        }
        return true;
    }

    space[key].setKey(INVALID_DFILEKEY);
    space[key].clearObject();
    if(freeKeys.unique_sorted_insert(key) == -1) {
        DL_ERROR(1, "key [%d] already is free", key);
        return false;
    }


    return true;
}
bool DLinearFileSystem::__push(DFile &file, DFileKey key) {

    int i = freeKeys.indexOf_bin(key);
    if(i != -1) {
        if(key < 0 || key >= space.size()) {
            DL_ERROR(1, "Bad key [%d] in freeKeys (pos: [%d])", key, i);
            return false;
        }
        if(space[key].isEmptyObject()) {
            //------------------------------- pushing in range
            freeKeys.removeByIndex(i);
            space[key] = file;
            file.setKey(key);
            //-------------------------------
        } else {
            DL_ERROR(1, "Uncleared file [%p] with free key: [%d]", space[key].descriptor(), key);
            return false;
        }
    } else if (key >= space.size()) {
        //------------------------------- pushing out of range
        space.reform(key + 1);
        space[key] = file;
        file.setKey(key);
        //-------------------------------
    } else {
        DL_ERROR(1, "Key [%d] is not free", key);
        return false;
    }

    return true;
}
void DLinearFileSystem::clear() {
    space.clear();
    freeKeys.clear();
}
bool DLinearFileSystem::registerFile(DFile &file) {
    return registerFile(file, file.key());
}
bool DLinearFileSystem::registerFile(DFile &file, DFileKey key) {

    if(BADFILE(file)) {
        DL_BADVALUE(1, "file");
        return false;
    }
    if(BADKEY(key)) {
        return __reg(file);
    } else {
        if( __push(file, key) == false ) {
            file.setKey(INVALID_DFILEKEY);
            return false;
        }
    }

    return true;
}
bool DLinearFileSystem::unregFile(DFileKey key) {

    return __unreg(key);
//    if(BADKEY(key)) {
//        DL_BADVALUE(1, "key: [%d]", key);
//        return false;
//    }
//    if(isKeyFree(key)) {
//        DL_ERROR(1, "Key [%d] is not registered", key);
//        return false;
//    }
//    if( freeKeys.unique_sorted_insert(key) == -1 ) {
//        DL_ERROR(1, "unique insert fail");
//        return false;
//    }
//    space[key].clearObject();

//    return true;
}
DFile DLinearFileSystem::file(DFileKey key) {
    return fileConstRef(key);
}
const DFile &DLinearFileSystem::fileConstRef(DFileKey key) const {

    if(BADKEY(key) || key >= space.size()) {
        return shared_null;
    }
    return space[key];
}
bool DLinearFileSystem::isKeyRegister(DFileKey key) const {

//    bool r = freeKeys.contain_bin(key);
//    DL_INFO(1, "LINEAR: key: [%d] size: [%d] freeKeys size: [%d] contain: [%d]",
//            key, space.size(), freeKeys.size(), r
//            );
    if(BADKEY(key) || key >= space.size()) {
        return false;
    }

    return !freeKeys.contain_bin(key);
}
int DLinearFileSystem::size() const {
    return space.size();
}
//=============================================================================== DMapFileSystem
void DMapFileSystem::clear() {
    space.clear();
}
bool DMapFileSystem::registerFile(DFile &file) {
    return registerFile(file, file.key());
}
bool DMapFileSystem::registerFile(DFile &file, DFileKey key) {


    if(BADFILE(file)) {
        DL_BADVALUE(1, "file");
        return false;
    }
    if(BADKEY(key)) {
        DL_BADVALUE(1, "key: [%d]", key);
        return false;
    }
    if(space.find(key) != space.end()) {
        DL_ERROR(1, "Key [%d] already register", key);
        return false;
    }
    space[key] = file;
    file.setKey(key);


    return true;
}
bool DMapFileSystem::unregFile(DFileKey key) {

    auto f = space.find(key);
    if(f == space.end()) {
        DL_ERROR(1, "Key [%d] is not register", key);
        return false;
    }
    f->second.setKey(INVALID_DFILEKEY);
    space.erase(f);
    return true;
}
DFile DMapFileSystem::file(DFileKey key) {
    return fileConstRef(key);
}
const DFile &DMapFileSystem::fileConstRef(DFileKey key) const {

    auto f = space.find(key);
    if(f != space.end()) {
        return f->second;
    }
    return shared_null;
}
bool DMapFileSystem::isKeyRegister(DFileKey key) const {
    if(space.find(key) == space.end()) {
        return false;
    }
    return true;
}
int DMapFileSystem::size() const {
    return space.size();
}












