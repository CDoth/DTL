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
DirReader::DirReader()
{
    use_prefix = false;
}
DirReader::DirReader(const char* _prefix)
{
    prefix = _prefix;
    use_prefix = true;
}
int DirReader::size()
{
    return directories.size();
}
void DirReader::set_prefix(const char* _prefix)
{
    prefix = _prefix;
    use_prefix = true;
}
void DirReader::enable_prefix()
{
    use_prefix = true;
}
void DirReader::disable_prefix()
{
    use_prefix = false;
}
const DirReader::dir* DirReader::add_dir(const char* _path, const char* _name)
{
    string_t name;
    string_t full_path;
    DIR *DIRECTORY = nullptr;
    struct dirent *ent = nullptr;
    //----------------------------------------------------------- Set name and full path:
    if(strlen(_name))
        name = _name;
    else
        name = _path;

    if(use_prefix)
        full_path = prefix + "\\" + _path;
    else
        full_path = _path;
    //----------------------------------------------------------- Check directories with same path:
    if(directories.size())
    {
        dir_iterator d_it = directories.begin();
        dir_iterator d_end = directories.end();
        while(d_it != d_end)
        {
            if( (*d_it)->full_path == full_path)
            {
                printf("DirReader::add_dir() : Directory with this path (%s) already open\n", full_path.c_str());
                return *d_it;
            }
            d_it++;
        }
    }
    //----------------------------------------------------------- Check directory to open:
    DIRECTORY = opendir(full_path.c_str());
    if(!DIRECTORY)
    {
        printf("DirReader::add_dir() Error: Can't open directory (%s), bad path\n", full_path.c_str());
        return nullptr;
    }
    //----------------------------------------------------------- Read names in current directory (except "." and ".." names):
    std::vector<string_t> pathes;
    std::vector<string_t> names;
    int c = 0;
    while( (ent = readdir(DIRECTORY)) )
    {
        if( !(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) )
        {
            string_t p = full_path + "\\" + ent->d_name;
            names.push_back(ent->d_name);
            pathes.push_back(full_path + "\\" + ent->d_name);

            c++;
        }
    }
    //----------------------------------------------------------- Create, init and push new dir struct:
    dir* new_dir = nullptr;
    new_dir = new dir;
    if(!new_dir)
    {
        printf("DirReader::add_dir() Error: Bad malloc result\n");
        return nullptr;
    }

    new_dir->name = name;
    new_dir->full_path = full_path;
    new_dir->size = c;
    new_dir->fnames = names;
    new_dir->fpathes = pathes;

    directories.push_back(new_dir);
    return new_dir;
}
const DirReader::dir* DirReader::get_dir_by_path(const char* _full_path)
{
    dir* lfr_dir = nullptr;
    if(directories.size())
    {
        dir_iterator d_it = directories.begin();
        dir_iterator d_end = directories.end();
        while(d_it != d_end)
        {
            if( (*d_it)->full_path == _full_path)
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
const DirReader::dir* DirReader::get_dir_by_name(const char* _name)
{
    dir* lfr_dir = nullptr;
    if(directories.size())
    {
        dir_iterator d_it = directories.begin();
        dir_iterator d_end = directories.end();
        while(d_it != d_end)
        {
            if( (*d_it)->name == _name)
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
DirReader::dir_iterator DirReader::begin()
{
    return directories.begin();
}
DirReader::dir_iterator DirReader::end()
{
    return directories.end();
}

#endif // DDIRREADER_H
