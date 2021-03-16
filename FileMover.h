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

    PlanFile = 2022,
    ForceRequest = 2032,
    AttachRequest = 2042,
    MultiPlanFile = 2052,
};
class FileMover
{
public:
    FileMover()
    {
        connection_status = false;
        message_buffer.resize(FM_DEFAULT_MESSAGE_BUFFER_SIZE);
        send_buffer.resize(FM_DEFAULT_SEND_BUFFER_SIZE);
        recv_buffer.resize(FM_DEFAULT_RECV_BUFFER_SIZE);
        string_buffer.resize(FM_DEFAULT_STRING_BUFFER_SIZE);

    }
    typedef std::string string;
    typedef uint64_t file_size_t;

    typedef uint32_t index_t;

    typedef uint32_t string_size;
    typedef uint32_t WORD;
    typedef uint64_t DWORD;
//private:
public:
    DirReader dir;
    int current_dir_index;
    DTcp control;
    bool connection_status;

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
    struct file_send_handler
    {
        bool interuptable;
        file_send_handler* connect_next; // multi-sending files
        file_send_handler* connect_prev; // multi-sending files
        int pack_freq; // multi-sending files
        //--------
        bool initial_sent;
        int pack_sent;



        file_info* f;
        index_t index;
        double prc;
        struct timeval start;
        size_t bytes_left;
        int flush_now;

        char speed[10];
        struct timeval last;
        size_t shift;
    };
    file_send_handler* alloc_send_handler()
    {
        file_send_handler* h = new file_send_handler;
        h->interuptable = true;
        h->connect_next = nullptr;
        h->connect_prev = nullptr;
        h->pack_freq = 1;
        h->initial_sent = false;
        h->pack_sent = 0;

        h->f = nullptr;
        h->index = -1;
        h->prc = 0.0;
        h->bytes_left = 0;
        h->flush_now = 0;
        h->shift = 0;

        return h;
    }
    struct file_recv_handler
    {
        int packet_size;
        int read_for_packet;


        file_info* f;
        int index;
        double prc;
        struct timeval start;
        size_t bytes_left;
        int read_now;

        int read;

        size_t shift;
        file_recv_handler* next;
        file_recv_handler* prev;
    };
    file_recv_handler* alloc_recv_handler()
    {
        file_recv_handler* h = new file_recv_handler;
        h->packet_size = 0;
        h->read_for_packet = 0;
        h->f = nullptr;
        h->index = 0;
        h->prc = 0.0;
        h->bytes_left = 0;
        h->read_now = 0;
        h->shift = 0;
        h->prev = nullptr;
        h->next = nullptr;
        return h;
    }
    struct specific_data_send_handler
    {
        header_type type;
        void* data;
    };



    typedef DArray<file_recv_handler*> file_list;
    file_list                                    recv_plan;

    std::map<index_t, file_send_handler*>        send_map;
    std::deque<file_send_handler*>               send_queue;

    std::map<index_t, file_recv_handler*>        recv_map;
    std::queue<index_t> available_index;

    std::queue<specific_data_send_handler*>      specific_queue;

    struct message_data
    {
        int begin;
        int size;
    };



    void add_dir(const char* path)
    {
        dir.add_dir(path);
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
    void chose_dir(int i)
    {
        current_dir_index = --i;
    }
    void print_dir(int i) const
    {
        --i;
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
        --i;
        auto d = dir.begin() + current_dir_index;
        auto file = (*d)->begin() + i;

        _debug << (*file).path().c_str();

        auto handler = prepare_to_send((*file).path().c_str());
        if(handler) push_new_file_handler(handler);
    }
    void return_index(index_t index)
    {
        if(index - system_index_shift < send_map.size() - 1 )
        {
            available_index.push(index);
        }
    }

    void insert_recv_handler(file_recv_handler* it, file_recv_handler* to)
    {
        if(to->next)
        {
            auto onext = to->next;
            to->next = it;
            it->prev = to;
            it->next = onext;
            onext->prev = it;
        }
        else
        {
            to->next = it;
            it->prev = to;
        }
    }
    void pull_recv_handler(file_recv_handler* h)
    {
//        _debug << "CALL: pull_recv_handler"<<h->index;
        if(h->next) h->next->prev = h->prev;
        if(h->prev) h->prev->next = h->next;
        h->next = nullptr;
        h->prev = nullptr;
    }
    void insert_send_handler(file_send_handler* it, file_send_handler* to)
    {
        if(to->connect_next)
        {
            auto onext = to->connect_next;
            to->connect_next = it;
            it->connect_prev = to;
            it->connect_next = onext;
            onext->connect_prev = it;
        }
        else
        {
            to->connect_next = it;
            to->connect_prev = it;
            it->connect_next = to;
            it->connect_prev = to;
        }
    }
    //---------------------------------------------------- restore
    void plan_to(file_recv_handler* to, file_recv_handler* h, const char* path = nullptr)
    {
        set_recv_path(h, path);
        update_plan(h);
        insert_recv_handler(h, to);
    }
    void plan_file(file_recv_handler* h, const char* path = nullptr)
    {
        auto _h = h;
        while(_h)
        {
            update_plan(_h);
            _h = _h->next;
        }
        set_recv_path(h, path);
        recv_plan.push_back(h);
    }
    void update_plan(file_recv_handler* h)
    {
//        _debug << "CALL: update_plan"<<h->index;
        for(int i=0;i!=recv_plan.size(); ++i)
        {
            file_recv_handler* plan_h = recv_plan[i];
            while(plan_h)
            {
//                _debug << "compare:" << "in plan:" << plan_h->index << "h:" << h->index;
                if(plan_h == h)
                {
//                    _debug << "remove:" << h->index;
                    if(plan_h == recv_plan[i])
                    {
                        if(plan_h->next)
                        {
                            recv_plan[i] = plan_h->next;
                            pull_recv_handler(plan_h);
                        }
                        else
                        {
                            recv_plan.remove(plan_h);--i;
                        }
                    }
                    else pull_recv_handler(plan_h);
                }
                plan_h = plan_h->next;
            }
        }
    }
    void print_plan() const
    {
        if(recv_plan.size())
        {
            auto b = recv_plan.constBegin();
            auto e = recv_plan.constEnd();
            file_recv_handler* h = nullptr;
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
    void push_force_request(index_t file_index, const char* path = nullptr)
    {
        set_recv_path(recv_map[file_index], path);
        specific_data_send_handler* sh;
        set_mem(sh,1);
        sh->type = ForceRequest;
        int* index = get_mem<int>(1);
        *index = file_index;
        sh->data = index;
        specific_queue.push(sh);
    }
    void push_file_request(file_recv_handler* h, const char* path = nullptr)
    {
        set_recv_path(h, path);
        specific_data_send_handler* sh;
        set_mem(sh,1);
        sh->type = FileRequest;
        sh->data = h;

        specific_queue.push(sh);
    }
    void push_attach_request(file_recv_handler* h, const char* path = nullptr)
    {
        set_recv_path(h, path);
        specific_data_send_handler* sh;
        set_mem(sh,1);
        sh->type = AttachRequest;
        sh->data = h;
        specific_queue.push(sh);
    }
    void push_multi_files_request(file_recv_handler* h, const char* path = nullptr)
    {
        if(!h->next)
        {
            push_file_request(h);
            return;
        }
        auto _h = h;
        while(_h)
        {
            set_recv_path(_h, path);
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
        auto b = recv_plan.begin();
        auto e = recv_plan.end();
        while( b != e ) push_multi_files_request(*b++, nullptr);
        recv_plan.clear();
    }
    void attach_files(const DArray<index_t>& files) //ms with current
    {
        if(send_queue.size())
        {
            file_send_handler* h = nullptr;
            file_send_handler* connect_to = send_queue.front();
            auto b = files.constBegin();
            auto e = files.constEnd();
            while( b != e )
            {
                h = send_map[*b];
                if(connect_to->connect_next)
                {
                    auto old_next = connect_to->connect_next;
                    h->connect_next = old_next;
                    h->connect_prev = connect_to;
                    old_next->connect_prev = h;
                    connect_to->connect_next = h;
                }
                else
                {
                    h->connect_next = connect_to;
                    h->connect_prev = connect_to;
                    connect_to->connect_prev = h;
                    connect_to->connect_next = h;
                }
                connect_to = h;
                ++b;
            }
        }
        else
        {
            push_ms_files(files);
        }
    }
    void push_file(file_send_handler* h)
    {
        send_queue.push_back(h);
    }
    void insert_file(index_t file_index)
    {
        send_queue.insert(send_queue.begin(), send_map[file_index]);
    }
    void push_files(const DArray<index_t>& files)
    {
        file_send_handler* h = nullptr;
        for(int i=0;i!= files.size();++i)
        {
            h = send_map[files[i]];
            send_queue.push_back(h);
        }
    }
    void push_ms_files(const DArray<index_t>& files)
    {
        file_send_handler* first = send_map[files.constFront()];
        if(files.size() > 1)
        {
            file_send_handler* last  = first;
            auto b = files.constBegin() + 1;
            auto e = files.constEnd();
            while( b != e )
            {
                auto curr = send_map[*b];
                last->connect_next = curr;
                curr->connect_prev = last;
                last = curr;
                ++b;
            }
            last->connect_next = first;
            first->connect_prev = last;
        }
        send_queue.push_back(first);
    }

    void set_recv_path(file_recv_handler* h, const char* path)
    {
        while(h)
        {
            if(path)
            {
                h->f->path = path;
                h->f->path.append(h->f->name);
            }
            else if(h->f->path.empty())
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
        if(!send_queue.size())
        {
            std::cout << "send queue is empty" << std::endl;
            return;
        }
        auto b = send_queue.begin();
        auto e = send_queue.end();
        while( b != e ) print_file_info((*b++)->f);
    }
    void push_new_file_handler(file_send_handler* f)
    {
        if(send_map.size())
        {
            specific_data_send_handler* sh;
            set_mem(sh, 1);
            sh->type = NewFileInSystem;
            sh->data = f;
            specific_queue.push(sh);
        }
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


        file_recv_handler* current_recv_file = nullptr;
        file_send_handler* current_send_file = nullptr;
        specific_data_send_handler* current_spec = nullptr;
        file_recv_handler* new_file = nullptr;
        file_recv_handler* start_recv_file = nullptr;

        file_send_handler* ms_handler = nullptr;

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
                                new_file = fm->alloc_recv_handler();
                                new_file->f = new file_info;
                                rc.add_item({&new_file->index, sizeof(index_t)});
                                rc.add_item({&new_file->f->size, sizeof(file_size_t)});
                                rc.add_item({&new_file->packet_size, sizeof(int)});
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
                                file_recv_handler* h = fm->recv_map[index];
                                gettimeofday(&h->start, nullptr);
                                if(h->shift)
                                    h->f->file = fopen(h->f->path.c_str(), "ab");
                                else
                                    h->f->file = fopen(h->f->path.c_str(), "wb");
                                h->read = 0;
                                h->prc  = 0.0;
                                h->bytes_left = h->f->size;
                                h->bytes_left -= h->shift;
                                h->read_for_packet = 0;
                                h->read_now = (size_t)h->packet_size < fm->recv_buffer.size ? h->packet_size : fm->recv_buffer.size;
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
                                fm->push_file(h);
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
                                if(ms_handler) fm->insert_send_handler(h, ms_handler);
                                else ms_handler = h;

                                if(rc.is_over())
                                {
                                    fm->push_file(ms_handler);
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
                            rb = fm->control.unlocked_recv_packet(fm->string_buffer.data, &packet_size);
                            if(packet_size && rb == packet_size)
                            {
                                int size = packet_size/sizeof(int);
                                files.reserve(size);
                                int* a = (int*)fm->string_buffer.data;
                                for(int i=0;i!=size;++i)
                                {
                                    files.push_back(a[i]);
                                }
                                fm->attach_files(files);
                                files.clear();
                                rtype = NoType;
                                data_index = 0;
                            }
                            break;
                        }
                        case ForceRequest:
                        {
                            rb = fm->control.unlocked_recv_to(&index, sizeof(index_t));
                            if(rb == sizeof(index_t))
                            {
                                rtype = NoType;
                                data_index = 0;
                                fm->insert_file(index);
                                current_send_file = nullptr;
                                index = 0;
                            }
                            break;
                        }
                        case NoType: break;
                        }
                    }
                    else
                    {
                        current_recv_file = fm->recv_map[data_index];
                        rb = fm->control.unlocked_recv_to(fm->recv_buffer.data, current_recv_file->read_now);
                        if(rb == current_recv_file->read_now)
                        {
                            fwrite(fm->recv_buffer.data, rb, 1, current_recv_file->f->file);
                            if( current_recv_file->bytes_left -= rb )
                            {
                                current_recv_file->read_for_packet += rb;
                                current_recv_file->prc = 100.0 - ((double) current_recv_file->bytes_left * 100.0 / current_recv_file->f->size);

                                std::cout << "recv progress: "<<current_recv_file->index << " " << current_recv_file->prc << " %" << std::endl;

                                current_recv_file->read_now = current_recv_file->bytes_left < (size_t)fm->recv_buffer.size ?
                                                              current_recv_file->bytes_left : fm->recv_buffer.size;
                                if(current_recv_file->read_for_packet == current_recv_file->packet_size)
                                {
                                    current_recv_file->read_for_packet = 0;
                                    data_index = 0;
                                    rtype = NoType;
                                }
                            }
                            else
                            {

                                timeval end;
                                gettimeofday(&end, nullptr);
                                timeval t = PROFILER::time_dif(&current_recv_file->start, &end);

                                std::cout << "recv file: " << current_recv_file->index << " " << current_recv_file->f->path <<
                                             "time: " << t.tv_sec << " sec " << t.tv_usec << " usec" << std::endl;

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
                if(  ((current_send_file && current_send_file->interuptable) || !current_send_file)  &&  fm->specific_queue.size())
                {
                    current_spec = fm->specific_queue.front();
                    switch (current_spec->type)
                    {
                    case NewFileInSystem:
                    {
                        if(sc.empty())
                        {
                            file_send_handler* h = (file_send_handler*)current_spec->data;
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
                            file_recv_handler* h = (file_recv_handler*)current_spec->data;
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
                            file_recv_handler* h = (file_recv_handler*)current_spec->data;
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
                        }
                        break;
                    }
                    case ForceRequest:
                    {
                        if(sc.empty())
                        {
                            int* index = (int*)current_spec->data;
                            sc.add_item({&nd_byte, sizeof(index_t), false});
                            sc.add_item({&current_spec->type, sizeof(header_type), false});
                            sc.add_item({index, sizeof(index_t), false});
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
                    case AttachRequest:
                    {
                        if(sc.empty())
                        {
                            DArray<int>* files = (DArray<int>*)current_spec->data;
                            int size = files->size();
                            if(size>7) sc.reserve(size + 3);
                            sc.add_item({&nd_byte, sizeof(index_t), false});
                            sc.add_item({&current_spec->type, sizeof(header_type), false});
                            size *= sizeof(int);
                            sc.add_item({files->constBegin(), size, true});
                            sc.complete();
                        }
                        if(sc.unlocked_send_all())
                        {
                            sc.reserve(10);
                            fm->specific_queue.pop();
                            delete (DArray<int>*)current_spec->data;
                            free_mem(current_spec);
                            current_spec = nullptr;
                        }
                        break;
                    }
                    case File: break;
                    case NoType: break;
                    }

                }
                if(!fm->send_queue.empty() && !current_spec)
                {
                    if(!current_send_file) current_send_file = fm->send_queue.front();
                    if(sc.empty() && !current_send_file->initial_sent)
                    {
                        current_send_file->interuptable = false;
                        current_send_file->initial_sent = false;
                        sc.add_item({&nd_byte, sizeof(index_t), false});
                        sc.add_item({&ft, sizeof(header_type), false});
                        sc.add_item({&current_send_file->index, sizeof(index_t), false});
                        current_send_file->prc = 0.0;
                        current_send_file->pack_sent = 0;
                        fseek(current_send_file->f->file, current_send_file->shift, SEEK_SET);
                        current_send_file->bytes_left -= current_send_file->shift;
                        current_send_file->flush_now = current_send_file->bytes_left < (size_t)fm->send_buffer.size ?
                                                       current_send_file->bytes_left : fm->send_buffer.size;
                        sc.complete();
                    }
                    if(sc.unlocked_send_all())
                    {
                        current_send_file->interuptable = true;
                        current_send_file->initial_sent = true;
                        gettimeofday(&current_send_file->last, nullptr);
                        gettimeofday(&current_send_file->start, nullptr);
                    }
                    if( current_send_file->initial_sent)
                    {
                        if(!file_byte_sent) file_byte_sent = fm->control.send_it(&current_send_file->index, sizeof(index_t)); //sizeof(index_t)
                        if(file_byte_sent == sizeof(index_t)) //sizeof(index_t)
                        {
                            if(current_send_file->interuptable)
                                fread(fm->send_buffer.data, current_send_file->flush_now, 1, current_send_file->f->file);
                            current_send_file->interuptable = false;
                            sb = fm->control.unlocked_send_it(fm->send_buffer.data, current_send_file->flush_now);

                            if(sb == current_send_file->flush_now)
                            {
                                file_byte_sent = 0;
                                if( current_send_file->bytes_left -= sb)
                                {
                                    current_send_file->flush_now = current_send_file->bytes_left < (size_t)fm->send_buffer.size ?
                                                                   current_send_file->bytes_left : fm->send_buffer.size;

                                    current_send_file->interuptable = true;
                                    current_send_file->prc = 100.0 - ((double) current_send_file->bytes_left * 100.0 / current_send_file->f->size);

                                    std::cout << "send progress: " << current_send_file->index << " " << current_send_file->prc << " %" <<std::endl;

                                    if(current_send_file->connect_next && ++current_send_file->pack_sent == current_send_file->pack_freq)
                                    {
                                        current_send_file->pack_sent = 0;
                                        current_send_file = current_send_file->connect_next;
                                    }
                                }
                                else
                                {
                                    timeval end;
                                    gettimeofday(&end, nullptr);
                                    timeval t = PROFILER::time_dif(&current_send_file->start, &end);

                                    std::cout << "sent file: " << current_send_file->index << " time: " << t.tv_sec << " sec " << t.tv_usec
                                              << "usec" << std::endl;

                                    fclose(current_send_file->f->file);
                                    delete current_send_file->f;
                                    if(current_send_file->connect_next)
                                    {
                                        if(current_send_file->connect_next == current_send_file->connect_prev)
                                        {
                                            current_send_file->connect_next->connect_next = nullptr;
                                            current_send_file->connect_next->connect_prev = nullptr;
                                        }
                                        else
                                        {
                                            current_send_file->connect_next->connect_prev = current_send_file->connect_prev;
                                            current_send_file->connect_prev->connect_next = current_send_file->connect_next;
                                        }
                                        fm->send_queue.front() = current_send_file->connect_next;
                                    }
                                    else
                                    {
                                        fm->send_queue.pop_front();
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
                        else current_send_file->interuptable = true;
                    }
                }
            }

            if(data_index == FM_DISCONNECT_BYTE) break;
        }


        std::cout << "Main stream is over" <<std::endl;
    }
public:

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
    file_recv_handler* find_handlers(const DArray<index_t>& num)
    {
        auto f = recv_map.begin();
        file_recv_handler* head = nullptr;
        for(int i=0;i!=num.size();++i)
        {
            if( ( f = recv_map.find(num[i]) ) != recv_map.end() )
            {
                auto h = f->second;
                update_plan(h);
                if(head)
                {
                    head->prev = h;
                    h->next = head;
                }
                head = h;
            }
        }
        return head;
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
                    auto f = recv_map.find(ii.num[i]);
                    if( f != recv_map.end() ) plan_file(f->second);
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
                file_recv_handler* head = find_handlers(ii.num);
                if(head) plan_file(head, ii.path);
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

                file_recv_handler* head = find_handlers(ii.num);
                if(head) plan_to(recv_map[to], head, ii.path);
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
                    auto f = recv_map.find(ii.num[i]);
                    if( f != recv_map.end() ) update_plan(f->second);
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
                    push_force_request(index);
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
                file_recv_handler* head = find_handlers(ii.num);
                if(head) push_attach_request(head, ii.path);
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
                file_recv_handler* head = find_handlers(ii.num);
                if(head) push_multi_files_request(head, ii.path);
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
                    if( f != recv_map.end() ) push_file_request(f->second);
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
                chose_dir(nd);
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
            prepare_to_send("G://DH//e1.mkv");
            prepare_to_send("G://DH//sv_test.mkv");
            prepare_to_send("G://DH//e2.mkv");
            prepare_to_send("G://DH//t1_.txt");
            prepare_to_send("G://DH//pic_car.jpg");
            prepare_to_send("G://DH//ost.zip");

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
                auto handler = prepare_to_send(value);
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
    file_send_handler* prepare_to_send(const char* path)
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

        file_send_handler* h = alloc_send_handler();
        h->f = new file_info;
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
//v 1.5.0
};


#endif // FILEMOVER_H
