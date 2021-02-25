#ifndef DDIRREADER_H
#define DDIRREADER_H

#include <vector>
#include <string>
#include <dirent.h>
class DirReader
{
public:
    DirReader();
    DirReader(const char* prefix);

    typedef std::string string_t;

    typedef struct
    {

        string_t full_path;
        string_t name;
        int size;

        typedef std::vector<string_t>::iterator iterator;
        iterator names_begin(){return fnames.begin();}
        iterator names_end(){return fnames.end();}
        iterator pathes_begin(){return fpathes.begin();}
        iterator pathes_end(){return fpathes.end();}

        std::vector<string_t> fpathes;
        std::vector<string_t> fnames;
    }dir;

    void set_prefix(const char* prefix);
    void enable_prefix();
    void disable_prefix();
    const dir* add_dir(const char* path, const char* name = "");
    const dir* get_dir_by_path(const char* full_path);
    const dir* get_dir_by_name(const char* name);

    typedef std::vector<dir*>::iterator dir_iterator;
    dir_iterator begin();
    dir_iterator end();
    int size();

public:
    string_t prefix;
    bool use_prefix;
    std::vector<dir*> directories;
};

#endif // DDIRREADER_H
