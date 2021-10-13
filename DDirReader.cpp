#include "DDirReader.h"
#include <algorithm>
#include "dmem.h"

#include <QDebug>

#define DDIR_LOG_IT(...) DLM_CLOG(log_context.header_set_message("default"), __VA_ARGS__);
#define DDIR_BADALLOC(...) DLM_CLOG(log_context.header_set_message("Can't alloc memory for"), __VA_ARGS__);
#define DDIR_BADPOINTER(...) DLM_CLOG(log_context.header_set_message("Bad pointer to"), __VA_ARGS__);
#define DDIR_BADVALUE(...) DLM_CLOG(log_context.header_set_message("Wrong value in"), __VA_ARGS__);
#define DDIR_FUNCFAIL(...) DLM_CLOG(log_context.header_set_message("Function fail"), __VA_ARGS__);

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
void getShortSize(size_t bytes, char *shortSizeBuffer, int bufferSize)
{
    if(bufferSize <= 0) return;
    double v = 0.0;
    for(;;)
    {
        int kb = bytes/ 1024;  bytes %= 1024; if(!kb){v = bytes; snprintf(shortSizeBuffer, bufferSize, "%.2f bytes", v);  break;}
        int mb = kb / 1024;    kb %= 1024;    if(!mb){v = kb + (double)bytes/1024; snprintf(shortSizeBuffer, bufferSize, "%.2f Kb", v);  break;}
        int gb = mb / 1024;    mb %= 1024;    if(!gb){v = mb + (double)kb/1024; snprintf(shortSizeBuffer, bufferSize, "%.2f Mb", v);  break;}
        int tb = gb / 1024;    gb %= 1024;    if(!tb){v = gb + (double)mb/1024; snprintf(shortSizeBuffer, bufferSize, "%.2f Gb", v); break;}
        v = tb + (double)gb/1024; snprintf(shortSizeBuffer, bufferSize, "%.2f Tb", v);   break;
    }
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
int path_correct(std::string &path)
{
    char sl = '/';
    for(size_t i=0;i!=path.size();++i)
    {
        if(path[i] == '/')
        {
            sl = '/';
            break;
        }
        if(path[i] == '\\')
        {
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
std::string path_get_last_section(const std::string &path)
{
    std::string::const_iterator iter = path.end();
    std::string::const_iterator b = path.begin();
    std::string::const_iterator e = path.end();
    while(b != e)
    {
        if(*b == '/' || *b == '\\')
            iter = b;
        ++b;
    }

    return std::string(iter+1, e);
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
Directory *FileDescriptor::getRoot()
{
    return root;
}
const Directory *FileDescriptor::getRoot() const
{
    return root;
}
//========================================== Directory
void Directory::addFile(const char *name)
{
    if(name)
    {
        FileDescriptor *file = new FileDescriptor;
        file->name = name;
        file->path = full_path + file->name;
        file->root = this;
        file->size = get_file_size(file->path.c_str());
        list.push_back(file);
    }
}
Directory::FileIterator Directory::beginFiles()
{
    return list.begin();
}
Directory::ConstFileIterator Directory::constBeginFiles() const
{
    return list.cbegin();
}
Directory::FileIterator Directory::endFiles()
{
    return list.end();
}
Directory::ConstFileIterator Directory::constEndFiles() const
{
    return list.cend();
}
std::string Directory::getStdPath() const
{
    return full_path;
}
std::string Directory::getStdName() const
{
    return name;
}
const char *Directory::getPath() const
{
    return full_path.c_str();
}
const char *Directory::getName() const
{
    return name.c_str();
}
size_t Directory::size() const
{
    return list.size();
}
FileDescriptor *Directory::getFile(size_t index)
{
    if(index >= 0 && index < list.size())
        return list[index];
    return NULL;
}
const FileDescriptor *Directory::getFile(size_t index) const
{
    if(index >= 0 && index < list.size())
        return list[index];
    return NULL;
}
FileDescriptor *Directory::getFile(const std::string &name)
{
    auto b = constBeginFiles();
    auto e = constEndFiles();
    while(b != e)
    {
        if((*b++)->name == name)
            return *b;
    }
    return NULL;
}
const FileDescriptor *Directory::getFile(const std::string &name) const
{
    auto b = constBeginFiles();
    auto e = constEndFiles();
    while(b != e)
    {
        if((*b++)->name == name)
            return *b;
    }
    return NULL;
}
Directory::Directory()
{

}
Directory::~Directory()
{

}
//========================================== DDirReader
DDirReader::DDirReader()
{
}
DDirReader::~DDirReader()
{
    auto b = begin();
    auto e = end();
    while( e != b ) delete (*--e);
}
size_t DDirReader::size() const
{
    return directories.size();
}

Directory *DDirReader::getDirectory(size_t index)
{
    if(index >= 0 && index < directories.size())
        return directories[index];
    return NULL;
}
const Directory *DDirReader::getDirectory(size_t index) const
{
    if(index >= 0 && index < directories.size())
        return directories[index];
    return NULL;
}
void DDirReader::init_log_context()
{
//    log_context = DLogs::DLogContext("DDirReaderLogs");
//    log_context.parse_console(true);
//    DLogs::DLogContext::header_resolution hr;
//    hr.cdate = false;
//    hr.fname = true;
//    hr.ltime = false;
//    hr.mnumb = true;
//    hr.rtime = false;
//    hr.sname = true;
//    log_context.header_set(hr);
}
Directory* DDirReader::open(const char* _path, const char* specific_name)
{
    if(_path == NULL)
    {
//        DDIR_BADPOINTER("Directory path");
        return NULL;
    }
    std::string name;
    std::string full_path;
    DIR *DIRECTORY = nullptr;
    struct dirent *ent = nullptr;
    //----------------------------------------------------------- Set name and full path:
    name = specific_name ? specific_name : _path;
    full_path = _path;
    path_correct(full_path);
    //----------------------------------------------------------- Check directories with same path:
    if(directories.size())
    {
        DirectoryIterator d_it = begin();
        DirectoryIterator d_end = end();
        while(d_it != d_end)
        {
            if( (*d_it)->getStdPath() == full_path)
            {
//                DDIR_LOG_IT("Directory with path [%s] already open", full_path.c_str());
                renew(full_path.c_str());
                return *d_it;
            }
            ++d_it;
        }
    }

    //----------------------------------------------------------- Check directory to open:
    DIRECTORY = opendir(full_path.c_str());
    if(DIRECTORY == NULL)
    {
//        DDIR_LOG_IT("Can't open directory [%s], bad path", full_path.c_str());
        return NULL;
    }

    //----------------------------------------------------------- Read names in current directory (except "." and ".." names):
    Directory *directory = new Directory;
    if(directory == NULL)
    {
//        DDIR_BADALLOC("new directory");
        return NULL;
    }

    directory->name = name;
    directory->full_path = full_path;
    while( (ent = readdir(DIRECTORY)) )
    {
        if( !(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) )
        {
#ifdef DDIR_STORE_FILE_SIZE
            size_t s = get_file_size( (full_path + ent->d_name).c_str() );
            if(s) fl.push_back({ent->d_name, s, new_dir}), ++c;
#else
            directory->addFile(ent->d_name);
#endif
        }
    }
    //----------------------------------------------------------- Create, init and push new dir struct:

    directories.push_back(directory);
    return directory;
}
int DDirReader::renew(const char *path_or_name)
{
    std::string key = path_or_name;
    Directory *target = NULL;
    DIR *DIRECTORY = NULL;
    struct dirent *ent = NULL;

    if(directories.size())
    {
        DirectoryIterator d_it = begin();
        DirectoryIterator d_end = end();
        while(d_it != d_end)
        {
            if( (*d_it)->getStdPath() == key || (*d_it)->getStdName() == key)
            {
                target = *d_it;
            }
            ++d_it;
        }
    }
    if(target)
    {
        DIRECTORY = opendir(target->full_path.c_str());
        if(DIRECTORY == NULL)
        {
//            DDIR_LOG_IT("Can't open directory [%s], bad path", target->full_path.c_str());
            return 0;
        }
        size_t old_size = target->size();
        target->list.clear();

        while( (ent = readdir(DIRECTORY)) )
        {
            if( !(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) )
            {
                target->addFile(ent->d_name);
            }
        }
        return target->size() - old_size;
    }

    return 0;
}
DDirReader::DirectoryIterator DDirReader::begin()
{
    return directories.begin();
}
DDirReader::ConstDirectoryIterator DDirReader::constBegin() const
{
    return directories.cbegin();
}
DDirReader::DirectoryIterator DDirReader::end()
{
    return directories.end();
}
DDirReader::ConstDirectoryIterator DDirReader::constEnd() const
{
    return directories.cend();
}


/*
DDirReader::dir_p DDirReader::get_dir_by_path(const char* _full_path)
{
    dir* lfr_dir = nullptr;
    if(directories.size())
    {
        dir_iterator d_it = directories.begin();
        dir_iterator d_end = directories.end();
        while(d_it != d_end)
        {
            if( (*d_it)->_full_path == _full_path)
            {
                lfr_dir = *d_it;
                break;
            }
            d_it++;
        }
        if(!lfr_dir)
            printf("DirReader::get_dir_by_path() Error: Can't find directory with path: %s\n", _full_path);
    }
    else
    {
        printf("DirReader::get_dir_by_path() Error: Empty directories list\n");
    }
    return lfr_dir;
}
DDirReader::dir_p DDirReader::get_dir_by_name(const char* _name)
{
    dir* lfr_dir = nullptr;
    if(directories.size())
    {
        dir_iterator d_it = directories.begin();
        dir_iterator d_end = directories.end();
        while(d_it != d_end)
        {
            if( (*d_it)->_name == _name)
            {
                lfr_dir = *d_it;
                break;
            }
            d_it++;
        }
        if(!lfr_dir)
            printf("DirReader::get_dir_by_path() Error: Can't find directory with name: %s\n", _name);
    }
    else
    {
        printf("DirReader::get_dir_by_path() Error: Empty directories list\n");
    }
    return lfr_dir;
}

DDirReader::dir_p DDirReader::get_dir_by_index(int index)
{
    return directories[index];
}
DDirReader::dir_p DDirReader::operator[](int index)
{
    return directories[index];
}
DDirReader::dir_p DDirReader::get_last_dir()
{
    return directories.back();
}
DDirReader::dir::iterator DDirReader::dir::find_file_by_name(const char *name, int with_extension) const //accelerate it
{
//    if(name == NULL) return end();
//    return std::find_if(file_list.begin(), file_list.end(), DDirReader::_find_file_f(name, bool(with_extension), true));
    for(size_t i=0;i!=file_list.size();++i)
    {
        if(file_list[i].name == name)
        {
            return file_list.begin() + i;
        }
    }
    return file_list.end();
}
bool DDirReader::dir::find_file_by_name(DDirReader::string_t name, int with_extension) const
{
    for(size_t i=0;i!=file_list.size();++i)
    {
        if(file_list[i].name == name)
        {
            return true;
        }
    }
    return false;
}
DDirReader::dir::iterator DDirReader::dir::find_file_by_path(const char *path, int with_extension) const
{
    if(path == NULL) return end();
    return std::find_if(file_list.begin(), file_list.end(), DDirReader::_find_file_f(path, bool(with_extension), false));
}

*/


#undef DDIR_LOG_IT
#undef DDIR_BADALLOC
#undef DDIR_BADPOINTER
#undef DDIR_BADVALUE
#undef DDIR_FUNCFAIL









