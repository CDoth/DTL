#ifndef DDIRREADER_H
#define DDIRREADER_H

#include <vector>
#include <string.h>
#include <dirent.h>
#include <fstream>

size_t get_file_size(const char* path);
size_t get_file_size(const std::string &path);
std::string getShortSize(size_t bytes);
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

int path_correct(std::string &path);
int path_cut_last_section(std::string &path);
int path_is_default(const std::string &path);
std::string path_get_last_section(const std::string &path);


//#define DDIR_STORE_FILE_SIZE
class Directory;
class DDirReader;
class FileDescriptor
{
public:
    FileDescriptor();
    ~FileDescriptor();

    std::string getStdName() const;
    std::string getStdPath() const;
    const char* getName() const;
    const char* getPath() const;
    int getSize() const;

    Directory* getRoot();
    const Directory* getRoot() const;
private:
    std::string path;
    std::string name;
    size_t size;
    Directory *root;
    friend class Directory;
};
class Directory
{
public:
    typedef std::vector<FileDescriptor*>::iterator FileIterator;
    typedef std::vector<FileDescriptor*>::const_iterator ConstFileIterator;

    FileIterator beginFiles();
    ConstFileIterator constBeginFiles() const;
    FileIterator endFiles();
    ConstFileIterator constEndFiles() const;

    std::string getStdPath() const;
    std::string getStdName() const;
    const char* getPath() const;
    const char* getName() const;
    size_t size() const;
    FileDescriptor* getFile(size_t index);
    const FileDescriptor* getFile(size_t index) const;

    FileDescriptor* getFile(const std::string &name);
    const FileDescriptor* getFile(const std::string &name) const;

    Directory();
    ~Directory();
    friend class DDirReader;
private:
    void addFile(const char *name);
    std::vector<FileDescriptor*> list;
    std::string full_path;
    std::string name;
};
class DDirReader
{
public:
    DDirReader();
    ~DDirReader();

    Directory* open(const char *path, const char *specific_name = nullptr);
    int renew(const char *path_or_name);

    typedef std::vector<Directory*>::iterator DirectoryIterator;
    typedef std::vector<Directory*>::const_iterator ConstDirectoryIterator;
    DirectoryIterator begin();
    ConstDirectoryIterator constBegin() const;

    DirectoryIterator end();
    ConstDirectoryIterator constEnd() const;
    size_t size() const;


    Directory* getDirectory(size_t index);
    const Directory* getDirectory(size_t index) const;
public:
    std::vector<Directory*> directories;

    void init_log_context();
};
/*
    class _find_file_f
    {
    public:
        _find_file_f(const char *key, bool with_ext, bool by_name) : _key (key), _with_ext(with_ext), _by_name(by_name) {}
        bool operator()(const file_desc &fd) const
        {
            if(_with_ext)
            {
                if(_by_name)
                    return fd.name == _key;
                else
                    return fd.path() == _key;
            }
            else
            {
                if(_by_name)
                    return getBaseName(fd.name) == _key;
                else
                    return getBaseName(fd.path()) == _key;
            }
        }
    private:
        const char *_key;
        bool _with_ext;
        bool _by_name;
    };
inline int check_file(const DDirReader::dir *_dir, const std::string &file_name)
{
    for(size_t i=0;i!=_dir->file_list.size();++i)
    {
        if(_dir->file_list[i].name == file_name)
        {
            return i;
        }
    }
    return -1;
}
*/

/* //test block
    DirReader dr;
    dr.set_prefix("/root");
    auto dir = dr.add_dir("DTL");
    printf("dr size: %d\n", dr.size());

    printf("dir: full_path: %s name: %s size: %d\n", dir->full_path.c_str(), dir->name.c_str(), dir->size);
    auto db = dir->begin();
    auto de = dir->end();
    int i=0;
    while( db != de )
    {
        printf("%d file: name: %s size: %d\n", i++, (*db).name().c_str(), (*db).size());
        ++db;
    }
 */
#endif // DDIRREADER_H
