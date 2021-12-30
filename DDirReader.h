#ifndef DDIRREADER_H
#define DDIRREADER_H

#include <vector>
#include <string.h>
#include <dirent.h>
#include <fstream>

#include <DArray.h>

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


class DDirectory;
class DDirReader;
class FileDescriptor {
public:
    FileDescriptor();
    ~FileDescriptor();

    std::string getStdName() const;
    std::string getStdPath() const;
    const char* getName() const;
    const char* getPath() const;
    int getSize() const;
    std::string getShortSize() const;
    int level() const;

    DDirectory* getRoot();
    const DDirectory* getRoot() const;
private:
    std::string path;
    std::string name;
    size_t size;
    std::string shortSize;
    DDirectory *root;
    friend class DDirectory;

    void *opaque;
};
typedef void*(*FileRegisterCallback)(FileDescriptor*, void*);
typedef void (*FileExcludeCallback)(FileDescriptor*, void*);
class DDirectory {
public:
    typedef std::vector<FileDescriptor*>::iterator FileIterator;
    typedef std::vector<FileDescriptor*>::const_iterator ConstFileIterator;


    bool open(const char *path, FileRegisterCallback reg = nullptr, void *regBase = nullptr);

    bool v_addDirectory(const char *path, FileRegisterCallback reg, void *regBase);
    bool v_addFile(const char *path,FileRegisterCallback reg, void *regBase);

    DDirectory *v_addVirtualDirectory(std::string name);




    std::string getStdPath() const;
    std::string getStdName() const;
    const char* getPath() const;
    const char* getName() const;
    int getFilesNumber() const;
    int getInnerDirsNumber() const;
    size_t getTotalSize() const;
    std::string getTotalShortSize() const;
    int getLevel() const;

    FileDescriptor* getFile(int index);
    const FileDescriptor* getFile(int index) const;

    DDirectory* getInnerDirectory(int index);
    const DDirectory* getInnerDirectory(int index) const;

    std::string topology();
    bool create(const std::string &topology);

    void clear();

    DDirectory();
    ~DDirectory();
    friend class DDirReader;
private:
    bool updateTopology(std::string &t);
    bool __create_lf_dirs(const std::string &topology, std::string::size_type &p);
    std::string __create_extract_name(const std::string &t, std::string::size_type &p);

    size_t initTotalSize();
    bool open(int l, FileRegisterCallback reg, void *regBase);
    bool addFile(const char *name, FileRegisterCallback reg, void *regBase);
    bool addInnerDirectory(const char *name);
    bool addObject(const char *name, FileRegisterCallback reg, void *regBase);


    bool innerAddVirtualFile(const char *path, FileRegisterCallback reg, void *regBase);

    DArray<FileDescriptor*> files;
    DArray<DDirectory*> innerDirs;

    bool bRootDir;
    bool bVirtual;
    int iLevel;

    size_t iTotalSize;
    std::string sTotalShortSize;
    std::string sFullPath;
    std::string sName;
};
class DDirReader {
public:
    DDirReader();
    ~DDirReader();

    void setFileRegistration(FileRegisterCallback reg, FileExcludeCallback exc, void *base);

    DDirectory* open(const char *path);
    DDirectory* createVirtual(const char *name);



    std::string topology();
    bool create(const std::string &topology);


    int size() const;
    void clear();


    DDirectory* getDirectory(int index);
    const DDirectory* getDirectory(int index) const;
private:
    DArray<DDirectory*> directories;

    FileRegisterCallback fileReg;
    FileExcludeCallback fileExclude;
    void *regBase;

};

#endif // DDIRREADER_H
