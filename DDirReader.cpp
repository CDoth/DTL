#include "DDirReader.h"
#include <algorithm>
#include "dmem.h"
#include <sys/stat.h>

#include <DLogs.h>

DLogs::DLogsContext log_context;
DLogs::DLogsContextInitializator logsInit("DDirReader", log_context);


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
std::string  getShortSize(size_t bytes) {
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
int path_correct(std::string &path) {
    char sl = '/';
    for(size_t i=0;i!=path.size();++i) {
        if(path[i] == '/') {
            sl = '/';
            break;
        }
        if(path[i] == '\\') {
            sl = '\\';
            break;
        }
    }
    if(path.back() != sl) path.push_back(sl);
    return path.size();
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

//    return std::string(iter+1, e);
}

//========================================== FileDescriptor
FileDescriptor::FileDescriptor()
{
    size = 0;
    root = NULL;
}
FileDescriptor::~FileDescriptor()
{

}
std::string FileDescriptor::getStdName() const
{
    return name;
}
std::string FileDescriptor::getStdPath() const
{
    return path;
}
const char *FileDescriptor::getName() const
{
    return name.c_str();
}
const char *FileDescriptor::getPath() const
{
    return path.c_str();
}
int FileDescriptor::getSize() const
{
    return size;
}
std::string FileDescriptor::getShortSize() const {
    return shortSize;
}
int FileDescriptor::level() const {
    if(root) {
        return root->getLevel() + 1;
    }
    return 0;
}
DDirectory *FileDescriptor::getRoot()
{
    return root;
}
const DDirectory *FileDescriptor::getRoot() const
{
    return root;
}

//========================================== Directory
bool DDirectory::addFile(const char *name, FileRegisterCallback reg, void *regBase) {

    if(name == nullptr) {
        DL_BADPOINTER(1, "name");
        return false;
    }
    FileDescriptor *file = new FileDescriptor;
    file->name = name;
    file->path = sFullPath + file->name;
    file->root = this;
    file->size = get_file_size(file->path.c_str());
//    file->size = 0;
    file->shortSize = getShortSize(file->size);
    files.push_back(file);

    if(reg) {
        file->opaque = reg(file, regBase);
    }

    return true;
}
bool DDirectory::addInnerDirectory(const char *name) {
    if(name == nullptr) {
        DL_BADPOINTER(1, "name");
        return false;
    }
    DDirectory *dir = new DDirectory;
    dir->sName = name;
    dir->sFullPath = sFullPath + dir->sName;
    path_correct(dir->sFullPath);
    dir->bRootDir = false;

    innerDirs.push_back(dir);

    return true;
}
bool DDirectory::addObject(const char *name, FileRegisterCallback reg, void *regBase) {

    std::string objectPath = sFullPath;
    objectPath.append(name);

    struct stat s;
    if( stat(objectPath.c_str(), &s) == 0) {
        if( s.st_mode & S_IFDIR) {
            if( !addInnerDirectory(name) ) {
                DL_FUNCFAIL(1, "addInnerDirectory");
                return false;
            }
        } else if (s.st_mode & S_IFREG) {
            if( !addFile(name, reg, regBase) ) {
                DL_FUNCFAIL(1, "addFile");
                return false;
            }
        } else {
            DL_ERROR(1, "Undefined object type");
            return false;
        }
    } else {
        DL_FUNCFAIL(1, "stat()");
        return false;
    }

    return true;
}
bool DDirectory::innerAddVirtualFile(const char *path, FileRegisterCallback reg, void *regBase) {

    FileDescriptor *file = new FileDescriptor;
    file->path = path;
    file->name = path_get_last_section(file->path);
    file->root = this;
    file->size = get_file_size(path);
    file->shortSize = getShortSize(file->size);
    files.push_back(file);

    if(reg) {
        file->opaque = reg(file, regBase);
    }

    return true;
}
bool DDirectory::open(const char *path, FileRegisterCallback reg, void *regBase) {

    if(path == nullptr) {
        DL_BADPOINTER(1, "path");
        return false;
    }
    if(bVirtual) {
        DL_ERROR(1, "Can't open real directory instead virtual");
        return false;
    }
    if(!bRootDir) {
        DL_ERROR(1, "Can't open inner directory manually");
        return false;
    }

    sFullPath = path;
    sName = path_get_last_section(sFullPath);

    DIR *DIRECTORY = nullptr;
    struct dirent *ent = nullptr;
    //=============================
    path_correct(sFullPath);
    //============
    DIRECTORY = opendir(sFullPath.c_str());
    if(DIRECTORY == nullptr) {
        DL_FUNCFAIL(1, "opendir");
        return false;
    }
    //=======================
    while( (ent = readdir(DIRECTORY)) ) {
        if( !(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) ) {
            addObject(ent->d_name, reg, regBase);
        }
    }

    FOR_VALUE(innerDirs.size(), i) {
        innerDirs[i]->open(iLevel + 1, reg, regBase);
    }

    initTotalSize();

    return true;
}
bool DDirectory::v_addDirectory(const char *path, FileRegisterCallback reg, void *regBase) {

    if(!bVirtual) {
        DL_ERROR(1, "Virtual functions works only with virtual directories");
        return false;
    }
    if(path == nullptr) {
        DL_BADPOINTER(1, "path");
        return false;
    }
    FOR_VALUE(innerDirs.size(), i) {
        if(innerDirs[i]->sFullPath == path) {
            DL_ERROR(1, "Directory [%s] already exist", path);
            return false;
        }
    }

    DDirectory *dir = new DDirectory;
    dir->open(path, reg, regBase);
    innerDirs.push_back(dir);

    return true;
}
bool DDirectory::v_addFile(const char *path, FileRegisterCallback reg, void *regBase) {

    if(!bVirtual) {
        DL_ERROR(1, "v_ functions works only with virtual directories");
        return false;
    }
    if(path == nullptr) {
        DL_BADPOINTER(1, "path");
        return false;
    }
    FOR_VALUE(files.size(), i) {
        if(files[i]->getStdPath() == path) {
            DL_ERROR(1, "File [%s] already exist", path);
            return false;
        }
    }
    if( !innerAddVirtualFile(path, reg, regBase) ) {
        DL_FUNCFAIL(1, "innerAddVirtualFile");
        return false;
    }
    return true;
}
DDirectory* DDirectory::v_addVirtualDirectory(std::string name) {

    if(!bVirtual) {
        DL_ERROR(1, "Virtual functions works only with virtual directories");
        return nullptr;
    }
    if(name.empty()) {
        DL_ERROR(1, "No name");
        return nullptr;
    }



    DDirectory *dir = new DDirectory;
    dir->bVirtual = true;
    dir->sName = name;
    dir->iLevel = iLevel + 1;
    innerDirs.push_back(dir);


//    std::cout << "CREATE VIRTUAL DIR: [" << name << "] " << (void*)dir
//              << " IN DIR: [" << sName << "] " << (void*)this
//              << std::endl;

    return dir;
}
bool DDirectory::open(int l, FileRegisterCallback reg, void *regBase) {

    if(sFullPath.size() && !bRootDir) {
        DIR *DIRECTORY = nullptr;
        struct dirent *ent = nullptr;
        //=============================
        path_correct(sFullPath);
        iLevel = l;
        //============
        DIRECTORY = opendir(sFullPath.c_str());
        if(DIRECTORY == nullptr) {
            DL_FUNCFAIL(1, "opendir");
            return false;
        }
        //=======================
        while( (ent = readdir(DIRECTORY)) ) {
            if( !(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) ) {
                addObject(ent->d_name, reg, regBase);
            }
        }
        FOR_VALUE(innerDirs.size(), i) {
            innerDirs[i]->open(iLevel + 1, reg, regBase);
        }
    }

    return true;
}


std::string DDirectory::getStdPath() const {
    return sFullPath;
}
std::string DDirectory::getStdName() const {
    return sName;
}
const char *DDirectory::getPath() const {
    return sFullPath.c_str();
}
const char *DDirectory::getName() const {
    return sName.c_str();
}
int DDirectory::getFilesNumber() const {
    return files.size();
}
int DDirectory::getInnerDirsNumber() const {
    return innerDirs.size();
}
size_t DDirectory::getTotalSize() const {
    return iTotalSize;
}
std::string DDirectory::getTotalShortSize() const {
    return sTotalShortSize;
}
int DDirectory::getLevel() const {
    return iLevel;
}
FileDescriptor *DDirectory::getFile(int index) {
    if(index >= 0 && index < files.size())
        return files[index];
    return nullptr;
}
const FileDescriptor *DDirectory::getFile(int index) const {
    if(index >= 0 && index < files.size())
        return files[index];
    return nullptr;
}
DDirectory *DDirectory::getInnerDirectory(int index) {
    if(index >= 0 && index < innerDirs.size()) {
        return innerDirs[index];
    }
    return nullptr;
}
const DDirectory *DDirectory::getInnerDirectory(int index) const {
    if(index >= 0 && index < innerDirs.size()) {
        return innerDirs[index];
    }
    return nullptr;
}
std::string DDirectory::topology(bool includeFiles) {

    std::string t;
    if( !__updateTopology(t, includeFiles) ) {
        return std::string();
    }

    return t;
}
bool DDirectory::create(const std::string &t) {
    if(innerDirs.size()) {
        DL_ERROR(1, "Directory already has topology, clear it before use create()");
        return false;
    }

//    std::cout << "DDirectory::create" << std::endl;
    bVirtual = true;
    std::string::size_type pos = 0;
    if( !__create_lf_dirs(t, pos) ) {
        DL_FUNCFAIL(1, "Main call: __create_lf_dirs");
        return false;
    }

    return true;
}
void DDirectory::clear() {
    FOR_VALUE(files.size(), i) {
        auto f = files[i];
        if(f) delete f;
    }
    files.clear();

    FOR_VALUE(innerDirs.size(), i) {
        auto dir = innerDirs[i];
        if(dir) delete dir;
    }
    innerDirs.clear();
}
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
            if( (currentDirectory = v_addVirtualDirectory(dirName)) == nullptr) {
                DL_FUNCFAIL(1, "v_addVirtualDirectory");
                return false;
            }
        } else if (s == 'O') {
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
            if(bRootDir) {
                sName = __create_extract_name(t, p);
                if(sName.empty()) {
                    DL_FUNCFAIL(1, "__create_extract_name (root name)");
                    return false;
                }
            } else {
                DL_ERROR(1, "Bad topology: bad pos for [R] marker");
                return false;
            }
        } else {
            DL_ERROR(1, "Bad topology: undefined marker: [%c] pos: [%d]", s, p);
            return false;
        }
        p = t.find('!', p);
    }

    return true;
}
std::string DDirectory::__create_extract_name(const std::string &t, std::string::size_type &p) {
    auto epos = t.find('/', p);
    if(epos != std::string::size_type(-1)) {
        std::string::const_iterator b = t.begin() + p;
        std::string::const_iterator e = t.begin() + epos;
        std::string dirName(b, e);

//        std::cout << "DIR: [" << dirName << "]"
//                  << " POS: [" << p << "]"
//                  << std::endl;
        p = epos;
        return dirName;
    } else {
        DL_ERROR(1, "Bad topology: no [/] as dir name end pos: [%d]", p);
        return std::string();
    }
}
bool DDirectory::__updateTopology(std::string &t, bool includeFiles) {

    if(innerDirs.empty()) {
        return true;
    }

    if(bRootDir) {
        t.append("!R");
        t.append(sName);
        t.append("/");
    }
    if(innerDirs.size() || (includeFiles && files.size())) {
        t.append("!I");
    }
    FOR_VALUE(innerDirs.size(), i) {
        DDirectory *d = innerDirs[i];
        t.append("!D");
        t.append(d->getName());
        t.append("/");
        d->__updateTopology(t, includeFiles);
    }
    if(includeFiles) {
        FOR_VALUE(files.size(), i) {
            FileDescriptor *fd = files[i];
            t.append("!F");
            t.append(fd->name);
            t.append("/");

        }
    }
    if(innerDirs.size() || (includeFiles && files.size())) {
        t.append("!O");
    }
    return true;

}

DDirectory::DDirectory() {
    bRootDir = true;
    bVirtual = false;
    iLevel = 0;
    iTotalSize = 0;
}
DDirectory::~DDirectory() {
    clear();
}




size_t DDirectory::initTotalSize() {

    iTotalSize = 0;
    FOR_VALUE(innerDirs.size(), i) {
        iTotalSize += innerDirs[i]->initTotalSize();
    }
    FOR_VALUE(files.size(), i) {
        iTotalSize += files[i]->getSize();
    }
    sTotalShortSize = getShortSize(iTotalSize);
    return iTotalSize;
}
//========================================== DDirReader
DDirReader::DDirReader()
{
    fileReg = nullptr;
    fileExclude = nullptr;
    regBase = nullptr;
}
DDirReader::~DDirReader()
{
}
void DDirReader::setFileRegistration(FileRegisterCallback reg, FileExcludeCallback exc, void *base) {
    fileReg = reg;
    fileExclude = exc;
    regBase = base;
}
int DDirReader::size() const
{
    return directories.size();
}
void DDirReader::clear() {
    FOR_VALUE(directories.size(), i) {
        auto dir = directories[i];
        if(dir) delete dir;
    }
    directories.clear();
}
DDirectory *DDirReader::getDirectory(int index)
{
    if(index >= 0 && index < directories.size())
        return directories[index];
    return NULL;
}
const DDirectory *DDirReader::getDirectory(int index) const
{
    if(index >= 0 && index < directories.size())
        return directories[index];
    return NULL;
}
DDirectory* DDirReader::open(const char* _path) {

    if(_path == nullptr) {
        return nullptr;
    }
    FOR_VALUE(directories.size(), i) {
        if(directories[i]->sFullPath == _path) {
            return directories[i];
        }
    }
    DDirectory *dir = new DDirectory;
    if( !dir->open(_path, fileReg, regBase) ) {
        delete dir;
    }
    directories.push_back(dir);
    return dir;
}
DDirectory *DDirReader::createVirtual(const char *name) {

    if(name == nullptr) {
        return nullptr;
    }
    FOR_VALUE(directories.size(), i) {
        if(directories[i]->sName == name) {
            return nullptr;
        }
    }

    DDirectory *vdir = new DDirectory;
    vdir->bVirtual = true;
    return vdir;
}
std::string DDirReader::topology() {
    std::string t;
    FOR_VALUE(directories.size(), i) {
        t.append("!X");
        std::string top = directories[i]->topology();
        if(top.empty()) {
            DL_FUNCFAIL(1, "DDirectory::topology");
            return std::string();
        }

        t.append(top);
        t.append("!Z");
    }

    return t;
}
bool DDirReader::create(const std::string &t) {

    std::string::size_type pos = 0;
    pos = t.find('!', pos);
    std::string::size_type term = std::string::size_type(-1);
    while(pos != term) {
        char s = t[pos+1];
        pos += 2;
        if(s == 'X') {
            std::string::const_iterator b = t.begin() + pos;
            if( (pos = t.find("!Z", pos)) == term ) {
                DL_ERROR(1, "Bad topology: can't find end of DDirectory topology ([!Z])");
                return false;
            }
            std::string::const_iterator e = t.begin() + pos;

            std::string localTop(b, e);
            DDirectory *dir = new DDirectory;
            dir->bVirtual = true;
            if( !dir->create(localTop) ) {
                DL_FUNCFAIL(1, "DDirectory::create");
                delete dir;
                return false;
            }
            directories.push_back(dir);

            pos += 2;

        } else {
            DL_ERROR(1, "Bad topology: Undefined marker: [%c]", s);
            return false;
        }
        pos = t.find('!', pos);
    }

    return true;
}














