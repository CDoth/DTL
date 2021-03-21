#ifndef FILEMOVER_H
#define FILEMOVER_H
#include "DTcp.h"
#include "DArray.h"
#include <fstream>
#include <queue>
#include "iostream"
#include "thread"
#include "DProfiler.h"
#include "DDirReader.h"
#include "daran.h"
#include "DLexeme.h"
#ifdef _WIN32
#include "QDebug"
#define _debug qDebug()
#else
#define debug std::cout
#endif

#define FM_DEFAULT_RECV_BUFFER_SIZE 1024 * 1024 * 4
#define FM_DEFAULT_SEND_BUFFER_SIZE 1024 * 1024 * 4
#define FM_MAX_RECV_BUFFER_SIZE 1024 * 1024 * 100
#define FM_MAX_SEND_BUFFER_SIZE 1024 * 1024 * 100

#define FM_DEFAULT_MESSAGE_BUFFER_SIZE 1024 * 1024
#define FM_DEFAULT_STRING_BUFFER_SIZE 1024 * 1024
#define FM_NEW_DATA_BYTE 0b10000000
#define FM_DISCONNECT_BYTE 0b01000000

enum header_type
{
    NewFileInSystem = 9009,
    File = 1001,
    MultiFiles = 1011,
    FileRequest = 2002,
    MultiFilesRequest = 2012,
    Message = 4004,
    NoType = 0,

    ForceRequest = 2032,
    AttachRequest = 2042,
};


class FileMover
{
public:
    FileMover()
    {
        connection_status = false;
        show_progress = true;
        message_buffer.resize(FM_DEFAULT_MESSAGE_BUFFER_SIZE);
        send_buffer.resize(FM_DEFAULT_SEND_BUFFER_SIZE);
        recv_buffer.resize(FM_DEFAULT_RECV_BUFFER_SIZE);
        string_buffer.resize(FM_DEFAULT_STRING_BUFFER_SIZE);
    }
    typedef std::string string;
    typedef uint64_t file_size_t;

    typedef int index_t;

    typedef uint32_t string_size;
    typedef uint32_t WORD;
    typedef uint64_t DWORD;
//private:
public:
    DirReader dir;
    int current_dir_index;
    DTcp control;
    bool connection_status;
    bool show_progress;

    const index_t system_index_shift = 10;
    //----------------------------------
    struct buffer
    {
        buffer() : data(nullptr), size(0), pos(0) {}
        ~buffer() {free_mem(data);}
        void resize(size_t s){size = s;reset_mem(data, size);}
        char* data;
        size_t size;
        size_t pos;
    };
    buffer message_buffer;
    buffer send_buffer;
    buffer recv_buffer;
    buffer string_buffer;
    string default_path;
    //-----------------------------------
    void set_default_download_path(const char* path) { default_path = path; }
    struct file_info
    {
        FILE* file;
        string path;
        string name;
        string shortSize;
        file_size_t size;
    };
    struct specific_data_send_handler
    {
        header_type type;
        void* data;
    };

    struct _send_info
    {
        bool interuptable;
        int pack_freq; // multi-sending files
        bool initial_sent;
        int pack_sent;
    };
    struct _recv_info
    {
        int packet_size;
        int read_for_packet;
    };

    struct file_handler
    {
        file_info* f;
        index_t index;
        size_t shift;
        file_handler* next;
        file_handler* prev;

        size_t bytes_left;
        int flush_now;
        double prc;
        struct timeval start;

        _send_info* si;
        _recv_info* ri;
    };

    file_handler* get_file_handler()
    {
        file_handler* h = get_mem<file_handler>(1);
        h->f = nullptr;
        h->index = 0;
        h->shift = 0;
        h->next = nullptr;
        h->prev = nullptr;
        h->bytes_left = 0;
        h->flush_now = 0;
        h->prc = 0.0;
        h->start = {0,0};
        h->si = nullptr;
        h->ri = nullptr;
        return h;
    }
    _recv_info* get_recv_info()
    {
        _recv_info* ri = get_mem<_recv_info>(1);
        ri->packet_size = 0;
        ri->read_for_packet = 0;
        return ri;
    }
    _send_info* get_send_info()
    {
        _send_info* si = get_mem<_send_info>(1);
        si->pack_freq = 0;
        si->pack_sent = 0;
        si->initial_sent = false;
        si->interuptable = false;
        return si;
    }



    struct file_multi_queue
    {
        DArray<file_handler*> list;
        void push(file_handler* h)
        {
            if(!h) {_debug << "file_multi_queue::push() : bad pointer to file handler"; return;}
            pull(h);
            list.push_back(h);
        }
        void pull(file_handler* h)
        {
            if(!h) {_debug << "file_multi_queue::pull() : bad pointer to file handler"; return;}
            if(contain(h))
            {
                for(int i=0;i!=list.size();++i)
                {
                    if(h == list[i])
                    {
                        if(h->next) list[i] = h->next;
                        else list.remove(h), --i;
                    }
                }
                disconnect_handler(h);
            }
        }
        void attach(file_handler* h, file_handler* to)
        {
            if(!h) {_debug << "file_multi_queue::attach() : bad pointer to file handler 1"; return;}
            if(!to) {_debug << "file_multi_queue::attach() : bad pointer to file handler 2"; return;}
            if(h == to) {_debug << "file_multi_queue::attach() : bad pointer to file handler 3"; return;}
            if(contain(to))
            {
                pull(h);
                if(to->next)
                {
                    auto onext = to->next;
                    to->next = h;
                    h->prev = to;
                    h->next = onext;
                    onext->prev = h;
                }
                else
                {
                    to->next = h;
                    h->prev = to;
                }
             }
            else
                _debug << "ATTACH: LIST HAS NO HANDLER:"<<to->index;
        }
        void attach_back(file_handler* h)
        {
            if(!h) {_debug << "file_multi_queue::attach_back() : bad pointer to file handler 1"; return;}
            if(!list.empty()) attach(h, list.back());
        }
        void attach_first(file_handler* h)
        {
            if(!h) {_debug << "file_multi_queue::attach_first() : bad pointer to file handler 1"; return;}
            if(!list.empty()) attach(h, list.front());
        }
        void ring(file_handler* h, file_handler* to)
        {
            if(!h) {_debug << "file_multi_queue::ring() : bad pointer to file handler 1"; return;}
            attach(h, to);
            if(h->next == nullptr && to->prev == nullptr)
            {
                h->next = to;
                to->prev = h;
            }
        }
        void ring_back(file_handler* h)
        {
            if(!h) {_debug << "file_multi_queue::ring_back() : bad pointer to file handler 1"; return;}
            if(!list.empty()) ring(h, list.back());
        }
        void ring_first(file_handler* h)
        {
            if(!h) {_debug << "file_multi_queue::ring_back() : bad pointer to file handler 1"; return;}
            if(!list.empty()) ring(h, list.front());
        }
        file_handler* handler_by_index(index_t i, const std::map<index_t, file_handler*>& map)
        {auto h = map.find(i); return h == map.end() ? nullptr : h->second;}
        inline bool contain(file_handler* h)
        {
            for(int i=0;i!=list.size();++i)
            {
                auto _h = list[i];
                do
                {
                    if(_h == h) return true;
                    _h = _h->next;
                }while(_h && _h != list[i]);
            }
            return false;
        }
    private:
        void disconnect_handler(file_handler* h)
        {
            if(h->next) h->next->prev = h->prev;
            if(h->prev) h->prev->next = h->next;
            h->next = nullptr;
            h->prev = nullptr;
        }
    };

    file_multi_queue                                recv_plan;
    file_multi_queue                                send_queue;

    std::map<index_t, file_handler*>                send_map;
    std::map<index_t, file_handler*>                recv_map;


    std::queue<index_t> available_index;
    std::queue<specific_data_send_handler*>      specific_queue;

    struct message_data
    {
        int begin;
        int size;
    };


    file_handler* recv_handler(index_t i)
    {auto h = recv_map.find(i); return h == recv_map.end() ? nullptr : h->second;}
    file_handler* send_handler(index_t i)
    {auto h = send_map.find(i); return h == send_map.end() ? nullptr : h->second;}

    void add_dir(const char* path)
    {
        if(path) dir.add_dir(path);
    }
    void print_dir_list() const
    {
        auto b = dir.begin();
        auto e = dir.end();
        int n=1;
        std::cout <<"Open directories:" <<std::endl;
        std::cout << "-----------------" << std::endl;
        while (b != e) std::cout << n++ << ": " << (*b++)->full_path << std::endl;
        std::cout << "-----------------" << std::endl;
    }
    void choose_dir(int i)
    {
        current_dir_index = --i;
    }
    void print_dir(int i) const
    {
        auto d = dir.begin() + i;
        auto b = (*d)->begin();
        auto e = (*d)->end();
        int n=1;
        std::cout << (*d)->full_path << ":" << std::endl;
        while(b != e)
        {
            std::cout << n++ << ": " << (*b).name() << " " << (*b).size() << std::endl;
            ++b;
        }
    }
    void print_current_dir() const
    {
        print_dir(current_dir_index);
    }
    void load_file_from_current_dir(int i)
    {
        auto d = dir.begin() + current_dir_index;
        auto file = (*d)->begin() + i;

        _debug << (*file).path().c_str();

        auto handler = load((*file).path().c_str());
        if(handler) push_new_file_handler(handler);
    }


    void return_index(index_t index)
    {
        if(index - system_index_shift < send_map.size() - 1 )
        {
            available_index.push(index);
        }
    }



    //---------------------------------------------------- restore
    void plan_to(file_handler* to, file_handler* h, const char* path)
    {
        if(!h) { _debug << "plan_to: bad recv handler"; return; }
        set_recv_path(h, path);
        recv_plan.attach(h, to);
    }
    void plan_file(file_handler* h, const char* path)
    {
        if(!h) { _debug << "plan_file: bad recv handler"; return; }
        set_recv_path(h, path);
        recv_plan.push(h);
    }
    void print_plan() const
    {
        if(recv_plan.list.size())
        {
            auto b = recv_plan.list.constBegin();
            auto e = recv_plan.list.constEnd();
            file_handler* h = nullptr;
            while( b != e )
            {
                h = *b;
                bool m = h->next ? true:false;
                if(m) std::cout << "(";
                while(h)
                {
                    std::cout << h->index;
                    if(h->next) std::cout << ", ";
                    h = h->next;
                }
                if(m) std::cout << ")";
                if(b != e-1) std::cout << ", ";
                ++b;
            }
            std::cout << std::endl;
        }
        else std::cout << "plan is empty" << std::endl;
    }
    void push_force_request(file_handler* h, const char* path)
    {
        if(!h) { _debug << "push_force_request: bad recv handler"; return; }
        recv_plan.pull(h);
        set_recv_path(h, path);
        specific_data_send_handler* sh;
        set_mem(sh,1);
        sh->type = ForceRequest;
        sh->data = h;
        specific_queue.push(sh);
    }
    void push_file_request(file_handler* h, const char* path)
    {
        if(!h) { _debug << "push_file_request: bad recv handler"; return; }
        recv_plan.pull(h);
        set_recv_path(h, path);
        specific_data_send_handler* sh;
        set_mem(sh,1);
        sh->type = FileRequest;
        sh->data = h;
        specific_queue.push(sh);

        if(h->f->size == h->shift)
        {
            recv_map.erase(recv_map.find(h->index));
            delete h->f;
            delete h;
        }
    }
    void push_attach_request(file_handler* h, const char* path)
    {
        if(!h)
        {
            _debug << "push_attach_request: bad pointer to file_recv_handler";
            return;
        }
        set_recv_path(h, path);
        specific_data_send_handler* sh;
        set_mem(sh,1);
        sh->type = AttachRequest;
        sh->data = h;
        specific_queue.push(sh);
    }
    void push_multi_files_request(file_handler* h, const char* path)
    {
        if(!h) { _debug << "push_multi_files_request: bad recv handler"; return; }
        if(!h->next)
        {
            push_file_request(h, path);
            return;
        }
        auto _h = h;
        while(_h)
        {
            set_recv_path(_h, path);
            recv_plan.pull(_h);
            _h = _h->next;
        }
        specific_data_send_handler* sh;
        set_mem(sh, 1);
        sh->type = MultiFilesRequest;
        sh->data = h;
        specific_queue.push(sh);
    }
    void start_plan()
    {
        while(!recv_plan.list.empty())
        {
            push_multi_files_request(recv_plan.list.front(), nullptr);
        }
        recv_plan.list.clear();
    }

    void free_file_handler(file_handler* h)
    {
        if(!h) if(!h) { _debug << "clear_local_file: bad recv handler"; return; }
        auto map_check = send_map.find(h->index);
        if(map_check != send_map.end()) send_map.erase(map_check);
        if((map_check = recv_map.find(h->index)) != recv_map.end()) recv_map.erase(map_check);
        delete h->f;
        free_mem(h->si);
        free_mem(h->ri);
        delete h;
    }
    void clear_completed()
    {
        for (auto it = recv_map.cbegin(); it != recv_map.cend();)
        {
            if(it->second->shift == it->second->f->size)
            {
                free_file_handler(it->second);
                recv_map.erase(it++);
//                it = recv_map.erase(it);
            }
            else ++it;
        }

//        for (auto it = recv_map.cbegin(), next_it = it; it != recv_map.cend(); it = next_it)
//        {
//          ++next_it;
//          if (it->second->shift == it->second->f->size) recv_map.erase(it);
//        }
    }
    void push_files(const DArray<index_t>& files)
    {
        file_handler* h = nullptr;
        for(int i=0;i!= files.size();++i)
        {
            h = send_map[files[i]];
            send_queue.push(h);
        }
    }
    void push_ms_files(const DArray<index_t>& files)
    {
        file_handler* first = send_map[files.constFront()];
        if(files.size() > 1)
        {
            file_handler* last  = first;
            auto b = files.constBegin() + 1;
            auto e = files.constEnd();
            while( b != e )
            {
                auto curr = send_map[*b];
                last->next = curr;
                curr->prev = last;
                last = curr;
                ++b;
            }
            last->next = first;
            first->prev = last;
        }
        send_queue.push(first);
    }
    void push_all(const char* path)
    {
        auto b = recv_map.begin();
        auto e = recv_map.end();
        while( b != e ) push_file_request(b->second, path), ++b;
    }
    void set_recv_path(file_handler* h, const char* path)
    {
        if(h->f->name.empty())
        {
            _debug << "set_recv_path: empty name:"<<h->index;
            return;
        }
        while(h)
        {
            if(path)
            {
                h->f->path = path;
                h->f->path.append(h->f->name);
            }
            else if(h->f->path.empty() && !default_path.empty())
            {
                h->f->path = default_path;
                h->f->path.append(h->f->name);
            }
            h->shift = get_file_size(h->f->path.c_str());
            h = h->next;
        }
    }
    //------------------------------------------------------------
    void print_file_info(const file_info* fi) const
    {
        std::cout << "File: " << fi->name << " " << fi->size << std::endl;
    }
    void print_local_table() const
    {
        if(!send_map.size())
        {
            std::cout << "local table is empty" << std::endl;
            return;
        }
        auto b = send_map.cbegin();
        auto e = send_map.cend();
        std::cout << "Local Table:" << std::endl;
        while( b!=e )
        {
            std::cout << b->first <<" : ";
            print_file_info(b->second->f);
            ++b;
        }
    }
    void print_out_table() const
    {
        if(!recv_map.size())
        {
            std::cout << "out table is empty" << std::endl;
            return;
        }
        auto b = recv_map.cbegin();
        auto e = recv_map.cend();
        std::cout << "Out Table:" << std::endl;
        while( b!=e )
        {
            std::cout << (int)b->first <<" : ";
            print_file_info(b->second->f);
            ++b;
        }
    }
    void print_send_queue() const
    {
        if(!send_queue.list.size())
        {
            std::cout << "send queue is empty" << std::endl;
            return;
        }
        auto b = send_queue.list.constBegin();
        auto e = send_queue.list.constEnd();
        while( b != e ) print_file_info((*b++)->f);
    }
    void push_new_file_handler(file_handler* f)
    {
        if(send_map.size())
        {
            specific_data_send_handler* sh;
            set_mem(sh, 1);
            sh->type = NewFileInSystem;
            sh->data = f;
            specific_queue.push(sh);
//            _debug << "push_new_file_handler: pushed:"<<f->index<<f->f->name.c_str();
        }
        else _debug << "push_new_file_handler: no files";
    }
    void push_message(int begin, int size)
    {
        specific_data_send_handler* sh;
        set_mem(sh, 1);
        sh->type = Message;
        message_data* md = get_mem<message_data>(1);
        md->begin = begin;
        md->size = size;
        sh->data = md;
        specific_queue.push(sh);
    }
    static void stream(FileMover* fm)
    {
        index_t nd_byte = FM_NEW_DATA_BYTE;
        index_t ds_byte = FM_DISCONNECT_BYTE;


        file_handler* current_recv_file = nullptr;
        file_handler* current_send_file = nullptr;
        specific_data_send_handler* current_spec = nullptr;
        file_handler* new_file = nullptr;
        file_handler* start_recv_file = nullptr;

        file_handler* ms_handler = nullptr;

        //----recv
        index_t data_index = 0;
        int rb = 0;
        header_type rtype = NoType;
        DArray<index_t> files;

        int ms_files = 0;
        int ms_size = 0;
        int table_size = 0;
        int table_item_packet = 0;
        int table_packets_recv = 0;
        int packet_size = 0;
        int index = 0;

        int recv_size = 0;


        //----send
        header_type ft = File;
        int speed_rate = 4;
        int sb = 0;

        int file_byte_sent = 0;

        void* send_place = nullptr;
        int send_size = 0;
        index_t recv_index = 0;
        size_t recv_shift = 0;


        SENDC sc(&fm->control);
        sc.reserve(10);
        RECVC rc(&fm->control);
        rc.reserve(10);
        //-----------------
        while(true)
        {
            if(!fm->connection_status) break;
            if(fm->control.check_readability(0,3000))
            {
                if(data_index)
                {
                    if(data_index == FM_NEW_DATA_BYTE)
                    {
                        if(rtype == NoType)
                        {
                            rb = fm->control.receive_to(&rtype, sizeof(header_type));
                        }
                        switch(rtype)
                        {
                        case NewFileInSystem:
                        {
                            if(!new_file)
                            {
                                new_file = fm->get_file_handler();
                                new_file->ri = fm->get_recv_info();
                                new_file->f = new file_info;
                                rc.add_item({&new_file->index, sizeof(index_t)});
                                rc.add_item({&new_file->f->size, sizeof(file_size_t)});
                                rc.add_item({&new_file->ri->packet_size, sizeof(int)});
                                zero_mem(fm->string_buffer.data, fm->string_buffer.size);
                                rc.add_item({fm->string_buffer.data, 0, true});
                                rc.complete();
                            }
                            if(rc.unlocked_recv_all())
                            {
                                new_file->f->name = fm->string_buffer.data;
                                fm->recv_map[new_file->index] = new_file;
                                new_file = nullptr;
                                rtype = NoType;
                                data_index = 0;
                                fm->print_out_table();
                            }
                            break;
                        }
                        case Message:
                        {
                            rb = fm->control.unlocked_recv_packet(fm->string_buffer.data, &packet_size);
                            if(packet_size && rb == packet_size)
                            {
                                data_index = 0;
                                rtype = NoType;
                                packet_size = 0;
                                std::cout << fm->string_buffer.data << std::endl;
                            }
                            break;
                        }
                        case File:
                        {
                            rb = fm->control.unlocked_recv_to(&index, sizeof(index_t));
                            if(rb == sizeof(index_t))
                            {
                                file_handler* h = fm->recv_map[index];
                                gettimeofday(&h->start, nullptr);
                                if(h->shift)
                                    h->f->file = fopen(h->f->path.c_str(), "ab");
                                else
                                    h->f->file = fopen(h->f->path.c_str(), "wb");
                                h->prc  = 0.0;
                                h->bytes_left = h->f->size;
                                h->bytes_left -= h->shift;
                                h->ri->read_for_packet = 0;
                                h->flush_now = (size_t)h->ri->packet_size < fm->recv_buffer.size ? h->ri->packet_size : fm->recv_buffer.size;
                                rtype = NoType;
                                data_index = 0;
                                index = 0;
                            }
                            break;
                        }
                        case FileRequest:
                        {
                            if(rc.empty())
                            {
                                rc.add_item({&recv_index, sizeof(index_t)});
                                rc.add_item({&recv_shift, sizeof(size_t)});
                                rc.complete();
                            }
                            if(rc.unlocked_recv_all())
                            {
                                auto h = fm->send_map[recv_index];
                                h->shift = recv_shift;
                                if(h->shift == h->f->size) fm->free_file_handler(h);
                                else fm->send_queue.push(h);
                                rtype = NoType;
                                data_index = 0;
                            }
                            break;
                        }
                        case MultiFilesRequest:
                        {
                            if(rc.empty())
                            {
                                rc.add_item({&recv_size, sizeof(int)});
                                rc.add_item({&recv_index, sizeof(index_t)});
                                rc.add_item({&recv_shift, sizeof(size_t)});
                                rc.add_script(2,1,&recv_size,true);
                                rc.complete();
                            }
                            if(rc.unlocked_recv_script())
                            {
                                auto h = fm->send_map[recv_index];
                                h->shift = recv_shift;
                                h->si->pack_freq = 1;


                                if(h->shift == h->f->size) fm->free_file_handler(h);
                                else if(ms_handler) fm->send_queue.ring_back(h);
                                else
                                {
                                    ms_handler = h;
                                    fm->send_queue.push(h);
                                }
                                if(rc.is_over())
                                {
                                    ms_handler = nullptr;
                                    rtype = NoType;
                                    data_index = 0;
                                    rc.clear();
                                }
                            }
                            break;
                        }
                        case AttachRequest:
                        {
                            if(rc.empty())
                            {
                                rc.add_item({&recv_size, sizeof(int)});
                                rc.add_item({&recv_index, sizeof(index_t)});
                                rc.add_item({&recv_shift, sizeof(size_t)});
                                rc.add_script(2,1,&recv_size,true);
                                rc.complete();
                            }
                            if(rc.unlocked_recv_script())
                            {
                                auto h = fm->send_map[recv_index];
                                h->shift = recv_shift;
                                h->si->pack_freq = 1;
//                                _debug << "get file for attach:"<<h<<h->index;
                                fm->send_queue.list.front()->si->pack_freq = 1;
                                if(h->shift == h->f->size) fm->free_file_handler(h);
                                else fm->send_queue.ring_first(h);
                                if(rc.is_over())
                                {
                                    ms_handler = nullptr;
                                    rtype = NoType;
                                    data_index = 0;
                                    rc.clear();
                                }
                            }
                            break;
                        }
                        case ForceRequest:
                        {
                            if(rc.empty())
                            {
                                rc.add_item({&recv_index, sizeof(index_t)});
                                rc.add_item({&recv_shift, sizeof(size_t)});
                                rc.complete();
                            }
                            if(rc.unlocked_recv_all())
                            {
                                auto h = fm->send_map[recv_index];
                                h->shift = recv_shift;
                                if(h->shift == h->f->size) fm->free_file_handler(h);
                                else fm->send_queue.list.push_front(h);
                                rtype = NoType;
                                data_index = 0;
                            }
                            break;
                        }
                        case NoType: break;
                        }
                    }
                    else
                    {
                        current_recv_file = fm->recv_map[data_index];
                        rb = fm->control.unlocked_recv_to(fm->recv_buffer.data, current_recv_file->flush_now);
                        if(rb == current_recv_file->flush_now)
                        {
                            fwrite(fm->recv_buffer.data, rb, 1, current_recv_file->f->file);
                            if( current_recv_file->bytes_left -= rb )
                            {
                                current_recv_file->ri->read_for_packet += rb;
                                current_recv_file->prc = 100.0 - ((double) current_recv_file->bytes_left * 100.0 / current_recv_file->f->size);

//                                if(fm->show_progress)
//                                std::cout << "recv progress: "<<current_recv_file->index << " " << current_recv_file->prc << " %" << std::endl;

                                current_recv_file->flush_now = current_recv_file->bytes_left < (size_t)fm->recv_buffer.size ?
                                                              current_recv_file->bytes_left : fm->recv_buffer.size;
                                if(current_recv_file->ri->read_for_packet == current_recv_file->ri->packet_size)
                                {
                                    current_recv_file->ri->read_for_packet = 0;
                                    data_index = 0;
                                    rtype = NoType;
                                }
                            }
                            else
                            {

                                timeval end;
                                gettimeofday(&end, nullptr);
//                                timeval t = PROFILER::time_dif(&current_recv_file->start, &end);

//                                std::cout << "recv file: " << current_recv_file->index << " " << current_recv_file->f->path <<
//                                             "time: " << t.tv_sec << " sec " << t.tv_usec << " usec" << std::endl;

                                fclose(current_recv_file->f->file);
                                fm->recv_map.erase(fm->recv_map.find(current_recv_file->index));
                                delete current_recv_file->f;
                                delete current_recv_file;
                                current_recv_file = nullptr;
                                data_index = 0;

                            }
                        }
                    }
                }
                else
                {
                    rb = fm->control.unlocked_recv_to(&data_index, sizeof(index_t));
                    if(rb < 0) break;
                    if(rb < (int)sizeof(index_t)) data_index = 0;
                }
            }
            if(fm->control.check_writability(0,3000))
            {
                if(!fm->connection_status)
                {
                    while( (sb = fm->control.send_it(&ds_byte, 1)) <= 0 );
                }
                if(  ((current_send_file && current_send_file->si->interuptable) || !current_send_file)  &&  fm->specific_queue.size())
                {
                    current_spec = fm->specific_queue.front();
                    switch (current_spec->type)
                    {
                    case NewFileInSystem:
                    {
                        if(sc.empty())
                        {
                            file_handler* h = (file_handler*)current_spec->data;
                            sc.add_item({&nd_byte, sizeof(index_t), false});
                            sc.add_item({&current_spec->type, sizeof(header_type), false});
                            sc.add_item({&h->index, sizeof(index_t), false});
                            sc.add_item({&h->f->size, sizeof(file_size_t), false});
                            sc.add_item({&h->flush_now, sizeof(int), false});
                            sc.add_item({h->f->name.c_str(), (int)h->f->name.size(), true});
                            sc.complete();
                        }
                        if(sc.unlocked_send_all())
                        {
                            fm->specific_queue.pop();
                            free_mem(current_spec);
                            current_spec = nullptr;

                        }
                        break;
                    }
                    case FileRequest:
                    {
                        if(sc.empty())
                        {
                            file_handler* h = (file_handler*)current_spec->data;
                            sc.add_item({&nd_byte, sizeof(index_t)});
                            sc.add_item({&current_spec->type, sizeof(header_type)});
                            sc.add_item({&h->index, sizeof(index_t)});
                            sc.add_item({&h->shift, sizeof(size_t)});
                            sc.complete();
                        }
                        if(sc.unlocked_send_all())
                        {
                            fm->specific_queue.pop();
                            free_mem(current_spec);
                            current_spec = nullptr;
                        }
                        break;
                    }
                    case Message:
                    {
                        if(sc.empty())
                        {
                            message_data* md = (message_data*)current_spec->data;
                            sc.add_item({&nd_byte, sizeof(index_t), false});
                            sc.add_item({&current_spec->type, sizeof(header_type), false});
                            sc.add_item({fm->message_buffer.data + md->begin, md->size, true});
                            sc.complete();
                        }
                        if(sc.unlocked_send_all())
                        {
                            fm->specific_queue.pop();
                            free_mem(current_spec->data);
                            free_mem(current_spec);
                            current_spec = nullptr;
                        }
                        break;
                    }
                    case MultiFilesRequest:
                    {
                        if(sc.empty())
                        {
                            file_handler* h = (file_handler*)current_spec->data;
                            sc.add_item({&nd_byte, sizeof(index_t), false});
                            sc.add_item({&current_spec->type, sizeof(header_type), false});

                            send_size = -1;
                            sc.add_item({&send_size, sizeof(int)});
                            while(h)
                            {
                                ++send_size;
                                sc.add_item({&h->index, sizeof(index_t)});
                                sc.add_item({&h->shift, sizeof(size_t)});
                                h = h->next;
                            }
                            sc.complete();
                        }
                        if(sc.unlocked_send_all())
                        {
                            sc.reserve(10);
                            fm->specific_queue.pop();
                            free_mem(current_spec);
                            current_spec = nullptr;
                            fm->clear_completed();
                        }
                        break;
                    }
                    case ForceRequest:
                    {
                        if(sc.empty())
                        {
                            file_handler* h = (file_handler*)current_spec->data;
                            sc.add_item({&nd_byte, sizeof(index_t)});
                            sc.add_item({&current_spec->type, sizeof(header_type)});
                            sc.add_item({&h->index, sizeof(index_t)});
                            sc.add_item({&h->shift, sizeof(size_t)});
                            sc.complete();
                        }
                        if(sc.unlocked_send_all())
                        {
                            fm->specific_queue.pop();
                            free_mem(current_spec);
                            current_spec = nullptr;
                        }
                        break;
                    }
                    case AttachRequest:
                    {
                        if(sc.empty())
                        {
                            file_handler* h = (file_handler*)current_spec->data;
                            sc.add_item({&nd_byte, sizeof(index_t), false});
                            sc.add_item({&current_spec->type, sizeof(header_type), false});

                            send_size = -1;
                            sc.add_item({&send_size, sizeof(int)});
                            while(h)
                            {
                                ++send_size;
                                sc.add_item({&h->index, sizeof(index_t)});
                                sc.add_item({&h->shift, sizeof(size_t)});
                                h = h->next;
                            }
                            sc.complete();
                        }
                        if(sc.unlocked_send_all())
                        {
                            sc.reserve(10);
                            fm->specific_queue.pop();
                            free_mem(current_spec);
                            current_spec = nullptr;
                            fm->clear_completed();
                        }
                        break;
                    }
                    case File: break;
                    case NoType: break;
                    }

                }
                if(!fm->send_queue.list.empty() && !current_spec)
                {
                    if(!current_send_file ||   (current_send_file->si->interuptable && current_send_file != fm->send_queue.list.front() )  )
                        current_send_file = fm->send_queue.list.front();
                    if(sc.empty() && !current_send_file->si->initial_sent)
                    {
                        current_send_file->si->interuptable = false;
                        current_send_file->si->initial_sent = false;
                        sc.add_item({&nd_byte, sizeof(index_t), false});
                        sc.add_item({&ft, sizeof(header_type), false});
                        sc.add_item({&current_send_file->index, sizeof(index_t), false});
                        current_send_file->prc = 0.0;
                        current_send_file->si->pack_sent = 0;
                        fseek(current_send_file->f->file, current_send_file->shift, SEEK_SET);
                        current_send_file->bytes_left -= current_send_file->shift;
                        current_send_file->flush_now = current_send_file->bytes_left < (size_t)fm->send_buffer.size ?
                                                       current_send_file->bytes_left : fm->send_buffer.size;
                        sc.complete();
                    }
                    if(sc.unlocked_send_all())
                    {
                        current_send_file->si->interuptable = true;
                        current_send_file->si->initial_sent = true;
//                        gettimeofday(&current_send_file->last, nullptr);
                        gettimeofday(&current_send_file->start, nullptr);
                    }
                    if( current_send_file->si->initial_sent )
                    {
                        if(!file_byte_sent) file_byte_sent = fm->control.send_it(&current_send_file->index, sizeof(index_t));
                        if(file_byte_sent == sizeof(index_t))
                        {
                            if(current_send_file->si->interuptable)
                                fread(fm->send_buffer.data, current_send_file->flush_now, 1, current_send_file->f->file);
                            current_send_file->si->interuptable = false;
                            sb = fm->control.unlocked_send_it(fm->send_buffer.data, current_send_file->flush_now);

                            if(sb == current_send_file->flush_now)
                            {
                                file_byte_sent = 0;
                                if( current_send_file->bytes_left -= sb )
                                {
                                    current_send_file->flush_now = current_send_file->bytes_left < (size_t)fm->send_buffer.size ?
                                                                   current_send_file->bytes_left : fm->send_buffer.size;

                                    current_send_file->si->interuptable = true;
                                    current_send_file->prc = 100.0 - ((double) current_send_file->bytes_left * 100.0 / current_send_file->f->size);

//                                    _debug << " -- next:"<<current_send_file->next << " -- prev:"<<current_send_file->prev<<" -- current:"<<current_send_file;
                                    if(fm->show_progress)
                                    std::cout << "send progress: " << current_send_file->index << " " << current_send_file->prc << " %" <<std::endl;

                                    if(current_send_file->next && ++current_send_file->si->pack_sent == current_send_file->si->pack_freq)
                                    {
                                        current_send_file->si->pack_sent = 0;
                                        current_send_file = current_send_file->next;
                                    }
                                }
                                else
                                {
                                    timeval end;
                                    gettimeofday(&end, nullptr);
//                                    timeval t = PROFILER::time_dif(&current_send_file->start, &end);

//                                    std::cout << "sent file: " << current_send_file->index << " time: " << t.tv_sec << " sec " << t.tv_usec
//                                              << "usec" << std::endl;

                                    fclose(current_send_file->f->file);
                                    delete current_send_file->f;
                                    if(current_send_file->next)
                                    {
                                        if(current_send_file->next == current_send_file->prev)
                                        {
                                            current_send_file->next->next = nullptr;
                                            current_send_file->next->prev = nullptr;
                                        }
                                        else
                                        {
                                            current_send_file->next->prev = current_send_file->prev;
                                            current_send_file->prev->next = current_send_file->next;
                                        }
                                        fm->send_queue.list.front() = current_send_file->next;
                                    }
                                    else
                                    {
                                        fm->send_queue.list.pop_front();
                                    }
                                    fm->return_index(current_send_file->index);
                                    fm->send_map.erase(fm->send_map.find(current_send_file->index));
                                    delete current_send_file;
                                    current_send_file = nullptr;
                                }


//                                struct timeval now;
//                                gettimeofday(&now, nullptr);
//                                int us = PROFILER::usec_dif(&current_send_file->last, &now);
//                                tsb += sb;
//                                if(us >= 1000000 / speed_rate)
//                                {
//                                    double v = 0;
//                                    tsb *= speed_rate;

//                                    for(;;)
//                                    {
//                                        int kb = tsb/ 1024;  tsb %= 1024; if(!kb){v = tsb; sprintf(current_send_file->speed, "%.2f bytes/s", v);  break;}
//                                        int mb = kb / 1024;    kb %= 1024;    if(!mb){v = kb + (double)tsb/1024; sprintf(current_send_file->speed, "%.2f Kb/s", v);  break;}
//                                        int gb = mb / 1024;    mb %= 1024;    if(!gb){v = mb + (double)kb/1024; sprintf(current_send_file->speed, "%.2f Mb/s", v);  break;}
//                                        int tb = gb / 1024;    gb %= 1024;    if(!tb){v = gb + (double)mb/1024; sprintf(current_send_file->speed, "%.2f Gb/s", v); break;}
//                                        v = tb + (double)gb/1024; sprintf(current_send_file->speed, "%.2f Tb", v);   break;
//                                    }

//                                    tsb = 0;
//                                    current_send_file->last = now;
//                                }


                            }
                        }
                        else current_send_file->si->interuptable = true;
                    }
                }
            }

            if(data_index == FM_DISCONNECT_BYTE) break;
        }


        std::cout << "Main stream is over" <<std::endl;
    }
public:
    file_handler* merge_handlers(const DArray<int>& list)
    {
        file_handler* head = nullptr;
        for(int i=0;i!=list.size();++i)
        {
            auto h = recv_handler(list[i]);
            recv_plan.pull(h);
            if(head) h->next = head;
            head = h;
        }
        return head;
    }

    struct intr_info
    {
        intr_info() : path(nullptr), unique_num(false) {}
        DArray<index_t> num;
        const char* path;
        DArray<char> opt;

        bool unique_num;
    };

    void read(intr_info* ii, const char* read_from)
    {
        const char* value = read_from;
        DArray<index_t>& list = ii->num;
        int index = 0;

        const char* lex_start = nullptr;
        int lex_size = 0;
        bool wait_num = true;

        enum lex_type {num = 1, opt = 2, other = 3};
        lex_type last_lex = other;

        char buffer[100];


        while(1)
        {
            if(*value != ' ' && *value != ',' && *value != '\0')
            {
                if(!lex_start) lex_start = value;
                if(lex_start) ++lex_size;
            }
            else
            {
                if(lex_start)
                {
                    last_lex = other;
                    if(*lex_start >= '0' && *lex_start <= '9' && wait_num)
                    {
                        index = atoi(lex_start);
                        wait_num = false;
                        last_lex = num;

                        if((ii->unique_num && list.count(index) == 0) || !ii->unique_num)
                            list.push_back(index);

                    }
                    if(*lex_start == '-')
                    {
                        char o = *(lex_start + 1);
                        last_lex = opt;
                        ii->opt.push_back(o);
                    }
                    if(last_lex == other)
                    {
                        snprintf(buffer, lex_size + 1, "%s", lex_start);
                        ii->path = lex_start;
                    }
                }
                if(*value == ',' && last_lex == num) wait_num = true;
                lex_start = nullptr;
                lex_size = 0;
            }


            if(*value == '\0') break;
            ++value;
        }
    }
    void disconnect()
    {
        connection_status = false;
    }
    void open_connection(int port)
    {
        control.make_server(port);
        control.wait_connection();
        control.unblock_out();
        connection_status = true;
        std::thread t(stream, this);
        t.detach();
    }
    void connect(int port, const char* address)
    {
        control.make_client(port, address);
        control.try_connect();
        control.unblock_out();
        connection_status = true;
        std::thread t(stream, this);
        t.detach();
    }
    void try_command(const char* command)
    {
        DArray<char> options;
        options.setMode(ShareWatcher);
        char cmd[30];
        const char* it = command;
        const char* value = nullptr;
        int cs = 0;
        while(*it != '\0')
        {
            if(*it == ':'){value = it+1;break;}
            ++cs;++it;
        }
        snprintf(cmd, cs+1, "%s", command);
        while(value && *value != '\0')
        {
            if(*value != ' ') break;
            ++value;
        }




        if(strcmp(cmd, "plan") == 0)
        {
            if(value)
            {
                intr_info ii;
                ii.opt = options;
                ii.opt.setMode(ShareWatcher);
                ii.unique_num = true;
                auto list = ii.num;
                read(&ii, value);
                for(int i=0;i!=ii.num.size();++i)
                {
//                    auto f = recv_map.find(ii.num[i]);
//                    if( f != recv_map.end() ) plan_file(f->second);
                }
            }
            else std::cout << "no value" << std::endl;
        }
        if(strcmp(cmd, "planp") == 0)
        {
            if(value)
            {
                intr_info ii;
                ii.opt = options;
                ii.opt.setMode(ShareWatcher);
                ii.unique_num = true;
                auto list = ii.num;
                read(&ii, value);
//                file_handler* head = merge_handlers(ii.num);
//                if(head) plan_file(head, ii.path);
            }
            else std::cout << "no value" << std::endl;
        }
        if(strcmp(cmd, "plan to") == 0)
        {
            if(value)
            {
                intr_info ii;
                ii.opt = options;
                ii.opt.setMode(ShareWatcher);
                ii.unique_num = true;
                read(&ii, value);
                index_t to = ii.num.front();
                ii.num.pop_front();

//                file_handler* head = merge_handlers(ii.num);
//                if(head) plan_to(recv_map[to], head, ii.path);
            }
            else std::cout << "no value" << std::endl;
        }
        if(strcmp(cmd, "planr") == 0)
        {
            if(value)
            {
                intr_info ii;
                ii.opt = options;
                ii.opt.setMode(ShareWatcher);
                ii.unique_num = true;
                auto list = ii.num;
                read(&ii, value);
                for(int i=0;i!=ii.num.size();++i)
                {
//                    auto f = recv_map.find(ii.num[i]);
//                    if( f != recv_map.end() ) update_plan(f->second);
                }
            }
            else std::cout << "no value" << std::endl;
        }
        if(strcmp(cmd, "get plan") == 0)
        {
            start_plan();
        }
        if(strcmp(cmd, "show plan") == 0)
        {
            print_plan();
        }
        //------------------------------------------ restore
        if(strcmp(cmd, "force") == 0)
        {
            if(value)
            {
                int index = atoi(value);
                if(recv_map.find(index) != recv_map.end())
                {
//                    push_force_request(index,nullptr);
                }
                else std::cout << "Wrong index: " << index << std::endl;
            }
            else std::cout << "no value" << std::endl;
        }
        if(strcmp(cmd, "attach") == 0)
        {
            if(value)
            {
                intr_info ii;
                ii.opt = options;
                ii.opt.setMode(ShareWatcher);
                ii.unique_num = true;
                auto list = ii.num;
                read(&ii, value);
//                file_handler* head = merge_handlers(ii.num);
//                if(head) push_attach_request(head, ii.path);
            }
            else std::cout << "no value" << std::endl;
        }
        if(strcmp(cmd, "getp") == 0)
        {
            if(value)
            {
                intr_info ii;
                ii.opt = options;
                ii.opt.setMode(ShareWatcher);
                ii.unique_num = true;
                auto list = ii.num;
                read(&ii, value);
//                file_handler* head = merge_handlers(ii.num);
//                if(head) push_multi_files_request(head, ii.path);
            }
            else std::cout << "no value" << std::endl;
        }
        if(strcmp(cmd, "get all") == 0)
        {
            const char* path = nullptr;
            if(value)
            {
                while(*value != '\0')
                {
                    if(*value != ' ')
                    {
                        path = value;
                        break;
                    }
                    ++value;
                }
            }
            auto b = recv_map.begin();
            auto e = recv_map.end();
            while( b != e ) push_file_request(b->second, path), ++b;
        }
        if(strcmp(cmd, "get") == 0)
        {
            if(value)
            {
                intr_info ii;
                ii.unique_num = true;
                ii.opt = options;
                ii.opt.setMode(ShareWatcher);
                auto list = ii.num;
                read(&ii, value);
                for(int i=0;i!=ii.num.size();++i)
                {
                    auto f = recv_map.find(ii.num.at(i));
                    if( f != recv_map.end() ) push_file_request(f->second, ii.path);
                }
            }
            else std::cout << "no value" << std::endl;
        }
        //--------------------------------------------------
        if(strcmp(cmd, "dir") == 0)
        {
        }
        if(strcmp(cmd, "dir add") == 0)
        {
            if(value)
            {
                intr_info ii;
                read(&ii, value);
                add_dir(ii.path);
            }
            else std::cout << "no value" << std::endl;
        }
        if(strcmp(cmd, "dir list") == 0)
        {
            print_dir_list();
        }
        if(strcmp(cmd, "dir chose") == 0)
        {
            if(value)
            {
                intr_info ii;
                read(&ii, value);
                int nd = ii.num.front();
                choose_dir(nd);
            }
        }
        if(strcmp(cmd, "dir show") == 0)
        {
            if(value)
            {
                intr_info ii;
                read(&ii, value);
                int nd = ii.num.front();
                print_dir(nd);
            }
        }
        if(strcmp(cmd, "dir load") == 0)
        {
            if(value)
            {
                intr_info ii;
                read(&ii, value);
                int i = ii.num.front();
//                _debug << "index:" << i;
                load_file_from_current_dir(i);
            }
        }


        if(strcmp(cmd, "info") == 0)
        {

        }
        if(strcmp(cmd, "test") == 0)
        {
            load("G://DH//e1.mkv");
            load("G://DH//sv_test.mkv");
            load("G://DH//e2.mkv");
            load("G://DH//t1_.txt");
            load("G://DH//pic_car.jpg");
            load("G://DH//ost.zip");

        }
        if(strcmp(cmd, "disconnect") == 0)
        {
            disconnect();
        }
        if(strcmp(cmd, "open connection") == 0)
        {
            if(value)
            {
                int port = atoi(value);
                if(port > 0 && port <= 65535)
                    open_connection(port);
                else std::cout << "Wrong port number: " << port << std::endl;
            }
            else std::cout << "no value" << std::endl;
        }
        if(strcmp(cmd, "connect to") == 0)
        {
            if(value)
            {
                int port = atoi(value);
                if(port > 0 && port <= 65535)
                {
                    const char* address = nullptr;
                    while(*value != '\0')
                    {
                        if(!address && *value == ' ') address = value;
                        if(address  && *value != ' ') {address = value; break;}
                        ++value;
                    }
                    connect(port, address);
                }
                else std::cout << "Wrong port number: " << port << std::endl;
            }
            else std::cout << "no value" << std::endl;
        }
        if(strcmp(cmd, "help") == 0)
        {

        }
        if(strcmp(cmd, "set defp") == 0)
        {
            if(value)
            {
                set_default_download_path(value);
            }
            else std::cout << "no value" << std::endl;
        }
        if(strcmp(cmd, "load") == 0)
        {
            if(value)
            {
                auto handler = load(value);
                if(handler) push_new_file_handler(handler);
            }
            else std::cout << "no value" << std::endl;
        }
        if(strcmp(cmd, "otable") == 0)
        {
            print_out_table();
        }
        if(strcmp(cmd, "ltable") == 0)
        {
            print_local_table();
        }


        if(strcmp(cmd, "m") == 0)
        {
            if(value)
            {
                int m_size = strlen(value)+1;
                memcpy(message_buffer.data + message_buffer.pos, value, m_size);
                push_message(message_buffer.pos, m_size);
                message_buffer.pos += m_size;
            }
        }
        if(strcmp(cmd, "set rbs") == 0)
        {
            if(value)
            {
                int size = atoi(value) * 1024 * 1024;
                if(size > 0 && size < FM_MAX_RECV_BUFFER_SIZE) recv_buffer.resize(size);
                else std::cout << "Wrong size: " << size << std::endl;
                std::cout << "Receive buffer size is: " << size << std::endl;
            }
        }
        if(strcmp(cmd, "set sbs") == 0)
        {
            if(value)
            {
                int size = atoi(value) * 1024 * 1024;
                if(size > 0 && size < FM_MAX_SEND_BUFFER_SIZE) send_buffer.resize(size);
                else std::cout << "Wrong size: " << size << std::endl;
                std::cout << "Send buffer size is: " << size << std::endl;
            }
        }
    }
    void trcm(const char* command)
    {
        auto l = lex.base(command, nullptr, " :");
        if(l.begin == nullptr) return;
        lex.default_context->set_min(0);
        lex.default_context->unique = true;

        DArray<int> list;
        DArray<char> options;
        list = lex.list(l.end);
        options = lex.options(l.end, nullptr);
        auto path = lex.lex(l.end,nullptr, "</ | \\> <$min:2>");
        show_progress = true;
//        std::cout << "list size: " << list.size() << " trcm base lexeme: ";
//        lex.print_lexeme(l);
//        std::cout << "path: ";
//        lex.print_lexeme(path);


        if(lex.base_equal(l, "dir"))
        {
            const char* pos = nullptr;
            if(lex.lex(l.end, nullptr, "<=open>").begin) //add dir
            {
                add_dir(path.begin);
            }
            if( (pos = lex.lex(l.end, nullptr, "<=choose>").begin)) //choose
            {
                int index = lex.number(pos, nullptr);
                if(index > 0 && index < dir.size())
                    choose_dir(lex.number(pos, nullptr));
            }
            if( (pos = lex.lex(l.end, nullptr, "<=list>").begin)) //print list
            {
                print_dir_list();
            }
            if( (pos = lex.lex(l.end, nullptr, "<=loadf>").begin)) //load file
            {
                auto DIR = dir.begin() + current_dir_index;
                int file_index = lex.number(l.end, nullptr) - 1;
                if(file_index > 0 && file_index < (*DIR)->size)
                {
                    auto handler = load((*DIR)->file_list[file_index].path().c_str());
                    if(handler) push_new_file_handler(handler);
                }
            }
            if( (pos = lex.lex(l.end, nullptr, "<=print>").begin)) //print dir
            {
                print_dir(lex.number(l.end, nullptr));
            }
            if(lex.lex(l.end, nullptr, "<=printc>").begin) //print current dir
            {
                print_dir(current_dir_index);
            }
        }
        //-----------------------------------
        if(lex.base_equal(l, "get"))
        {
//            qDebug() << "COMMAND: GET";
            if(lex.lex(l.end,nullptr, "<=all>").begin)
                push_all(path.begin);

            else if(lex.lex(l.end, nullptr, "<=plan>").begin) start_plan();
            else for(int i=0;i!=list.size();++i)
                    push_file_request(recv_handler(list[i]), path.begin);
        }
        if(lex.base_equal(l, "getp"))
        {
//            qDebug() << "COMMAND: GETP";
            push_multi_files_request(merge_handlers(list), path.begin);
        }
        if(lex.base_equal(l, "attach"))
        {
//            qDebug() << "COMMAND: ATTACH";
            push_attach_request(merge_handlers(list), path.begin);
        }
        if(lex.base_equal(l, "force"))
        {
//            qDebug() << "COMMAND: FORCE";
            int index = lex.number(l.end,nullptr);

            push_force_request(recv_handler(index),path.begin);
        }
        //-------------------------------------
        if(lex.base_equal(l, "plan"))
        {
//            qDebug() << "COMMAND: PLAN";
            if(lex.lex(l.end, nullptr, "<=to>").begin)
            {
                int to = list.front();
                list.pop_front();                
                auto _to = recv_handler(to);
                for(int i=0;i!=list.size();++i)
                    plan_to(_to, recv_handler(list[i]), path.begin);
            }
            else
                for(int i=0;i!=list.size();++i)
                    plan_file(recv_handler(list[i]), path.begin);
        }
        if(lex.base_equal(l, "planp"))
        {
//            qDebug() << "COMMAND: PLANP";
            if(!list.empty())
            {
                plan_file(recv_handler(list.front()), path.begin);
                list.pop_front();
            }
            for(int i=0;i!=list.size();++i)
                plan_to(recv_plan.list.back(), recv_handler(list[i]), path.begin);
        }
        if(lex.base_equal(l, "planr"))
        {
//            qDebug() << "COMMAND: PLANR";
            for(int i=0;i!=list.size();++i)
                recv_plan.pull(recv_handler(list[i]));
        }
        if(lex.base_equal(l, "show"))
        {
//            qDebug() << "COMMAND: SHOW";
            if(lex.lex(l.end, nullptr, "<=plan>").begin)
            {
                print_plan();
            }
            else if(lex.lex(l.end, nullptr, "<=otable>").begin)
            {
                print_out_table();
            }
            else if(lex.lex(l.end, nullptr, "<=ltable>").begin)
            {
                print_local_table();
            }
        }
        //---------------------------------
        if(lex.base_equal(l, "load"))
        {
            auto handler = load(path.begin);
            if(handler) push_new_file_handler(handler);
        }
        if(lex.base_equal(l, "m"))
        {
//            qDebug() << "COMMAND: M";

            const char* mes = lex.lex(l.end, nullptr, "<$min:1>").begin;
            if(mes)
            {
                int m_size = strlen(mes) + 1;
                memcpy(message_buffer.data + message_buffer.pos, mes, m_size);
                push_message(message_buffer.pos, m_size);
                message_buffer.pos += m_size;
            }

        }

        if(lex.base_equal(l, "test"))
        {
            trcm("load G:/DH/e1.mkv");
            trcm("load G:/DH/e2.mkv");
        //    trcm("load G:/DH/gz.mkv");
        //    trcm("load G:/DH/sv_test.mkv");
            trcm("load G:/DH/t1.txt");
            trcm("load G:/DH/ost.zip");
            trcm("load G:/DH/pic_car.jpg");
        }
        for(int i=0;i!=options.size();++i)
        {
            switch(options[i])
            {
            case 'v': show_progress = false; break;
            case 'p': show_progress = true; break;
            }
        }

    }
    file_handler* load(const char* path)
    {
        auto b = send_map.cbegin();
        while( b != send_map.cend() )
        {
            if( strcmp( b->second->f->path.c_str(), path) == 0 )
            {
                std::cout << "File already in system:" << path << std::endl;
                return nullptr;
            }
            ++b;
        }
        FILE* file = fopen(path, "rb");
        if(!file)
        {
            std::cout << "Wrong path" << std::endl;
            return nullptr;
        }
        const char* pi = path;
        const char* pn = path;
        while(*pi != '\0')
        {
            if(*pi == '/' || *pi == '\\') pn = pi+1;
            ++pi;
        }
        size_t bytes = get_file_size(path);
        size_t size = bytes;

        char shortSize[10];
        double v = 0.0;
        for(;;)
        {
            int kb = bytes/ 1024;  bytes %= 1024; if(!kb){v = bytes; sprintf(shortSize, "%.2f bytes", v);  break;}
            int mb = kb / 1024;    kb %= 1024;    if(!mb){v = kb + (double)bytes/1024; sprintf(shortSize, "%.2f Kb", v);  break;}
            int gb = mb / 1024;    mb %= 1024;    if(!gb){v = mb + (double)kb/1024; sprintf(shortSize, "%.2f Mb", v);  break;}
            int tb = gb / 1024;    gb %= 1024;    if(!tb){v = gb + (double)mb/1024; sprintf(shortSize, "%.2f Gb", v); break;}
            v = tb + (double)gb/1024; sprintf(shortSize, "%.2f Tb", v);   break;
        }

        file_handler* h = get_file_handler();
        h->f = new file_info;
        h->si = get_send_info();
        h->f->path = path;
        h->f->name = pn;
        h->f->shortSize = shortSize;
        h->f->size = size;
        h->f->file = file;
        if(available_index.size())
        {
            h->index = available_index.front();
            available_index.pop();
        }
        else
        {
            h->index = send_map.size() + system_index_shift;
        }

        h->bytes_left = size;
        h->flush_now = h->bytes_left < (size_t)send_buffer.size ? h->bytes_left : send_buffer.size;
        send_map[h->index] = h;
        print_file_info(h->f);
        return h;
    }
private:
    DLexeme lex;
//v 1.5.0
};


#endif // FILEMOVER_H
