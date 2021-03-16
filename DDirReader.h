#ifndef DDIRREADER_H
#define DDIRREADER_H

#include <vector>
#include <string>
#include <dirent.h>
#include <fstream>

size_t get_file_size(const char* path);
class DirReader
{
public:
    DirReader();
    DirReader(const char* prefix);

    typedef std::string string_t;

    struct dir;
    struct file_desc
    {
    private:
        string_t _name;
        size_t   _size;
        dir* pdir;

    public:
        file_desc() : _size(0), pdir(nullptr) {}
        file_desc(const char* name, size_t size, dir* parent) : _name(name), _size(size), pdir(parent) {}
        string_t name() const;
        string_t path() const;
        size_t size() const;
    };

    struct dir
    {
        string_t full_path;
        string_t name;
        int size;

        typedef std::vector<file_desc>::const_iterator iterator;
        std::vector<file_desc> file_list;

        iterator begin() const {return file_list.cbegin();}
        iterator end() const {return file_list.cend();}
    };

    void set_prefix(const char* prefix);
    void enable_prefix();
    void disable_prefix();
    const dir* add_dir(const char* path, const char* specific_name = nullptr);
    const dir* get_dir_by_path(const char* full_path);
    const dir* get_dir_by_name(const char* name);

    typedef std::vector<dir*>::const_iterator dir_iterator;
    dir_iterator begin() const;
    dir_iterator end() const;
    int size() const;
public:
    dir* current;
    string_t prefix;
    bool use_prefix;
    std::vector<dir*> directories;
};

#endif // DDIRREADER_H
