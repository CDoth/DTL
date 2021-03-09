#ifndef FILEMOVER_H
#define FILEMOVER_H
#include "DTcp.h"
#include "DArray.h"
#include <fstream>
#include <queue>
#include "iostream"
#include "thread"
#include "DProfiler.h"
#include <mutex>

#define FM_DEFAULT_RECV_BUFFER_SIZE 1024 * 1024 * 5
#define FM_DEFAULT_SEND_BUFFER_SIZE 1024 * 1024 * 5

#define FM_DEFAULT_MESSAGE_BUFFER_SIZE 1024 * 1024
#define FM_DEFAULT_STRING_BUFFER_SIZE 1024 * 1024
#define FM_DEFAULT_DOWNLOAD_PATH "C://LOAD//"
#define FM_NEW_DATA_BYTE 0b10000000
#define FM_DISCONNECT_BYTE 0b01000000
enum header_type {Table = 9009, File = 1001, MultiFiles = 1011, FileRequest = 2002, MultiFilesRequest = 2012, TableRequest = 3003, Message = 4004, NoType = 0};


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

        set_default_download_path(FM_DEFAULT_DOWNLOAD_PATH);
    }
    typedef std::string string;
//private:
public:
    DTcp control;
    bool connection_status;
    std::mutex mu;

    uint8_t last_ms_file = 0b10000001;
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
        size_t size;
        std::vector<file_info*>::const_iterator place;
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
        int index;
        uint8_t multisending_index;
        double prc;
        struct timeval start;
        size_t bytes_left;
        int flush_now;

        char speed[10];
        struct timeval last;

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
        h->multisending_index = last_ms_file++;
        h->prc = 0.0;
        h->bytes_left = 0;
        h->flush_now = 0;
        return h;
    }
    struct file_recv_handler
    {
        int packet_size;
        int read_for_packet;



        file_info* f;
        int index;
        uint8_t multisending_index;
        double prc;
        struct timeval start;
        size_t bytes_left;
        int read_now;


        int read;
    };
    file_recv_handler* alloc_recv_handler()
    {
        file_recv_handler* h = new file_recv_handler;
        h->packet_size = 0;
        h->read_for_packet = 0;
        h->f = nullptr;
        h->index = 0;
        h->multisending_index = last_ms_file++;
        h->prc = 0.0;
        h->bytes_left = 0;
        h->read_now = 0;
        return h;
    }
    struct specific_data_send_handler
    {
        header_type type;
        void* data;
    };



    typedef DArray<file_info*> FILE_TABLE;
    //send:
    FILE_TABLE                                   local_table;
    std::queue<file_send_handler*>               files_queue;
    //recv:
    FILE_TABLE                                   out_table;
    std::map<uint8_t, file_recv_handler*>        recv_files;


    std::queue<specific_data_send_handler*>      specific_queue;

    struct send_item
    {
        const void* data;
        int size;
        bool packet;
    };
    struct message_data
    {
        int begin;
        int size;
    };
    struct mfiles_data
    {
        DArray<int> files;
    };


    void print_file_info(const file_info* fi) const
    {
        std::cout << "File: " << fi->path << " " << fi->shortSize << std::endl;
    }
    void print_local_table() const
    {
        if(local_table.size())
        {
            std::cout << "Local table:" << std::endl;
            for(int i=0;i!=local_table.size();++i)
            {
                std::cout << i <<" : ";
                print_file_info(local_table[i]);
            }

        }
        else qDebug()<<"Local table is empty";
    }
    void print_out_table() const
    {
        if(out_table.size())
        {
            std::cout << "Out table:" << std::endl;
            for(int i=0;i!=out_table.size();++i)
                std::cout << i << ": " << out_table[i]->name << " " << out_table[i]->shortSize << std::endl;
        }
        else qDebug()<<"Out table is empty";
    }

    void push_files(const DArray<int>& files)
    {
        qDebug()<<"push files"<<files.size();
        int first = files.constFront();
        file_send_handler* fh = alloc_send_handler();
        fh->f = local_table[first];
        fh->pack_freq = 1;
        fh->index = first;
        file_send_handler* curr = fh;
        auto b = files.constBegin() + 1;
        auto e = files.constEnd();
        while( b != e )
        {
            file_send_handler* h = alloc_send_handler();
            h->f = local_table[*b];
            h->pack_freq = 1;
            h->index = *b;
            curr->connect_next = h;
            h->connect_prev = curr;
            h->bytes_left = 0;
            curr = h;
            ++b;
        }
        fh->connect_prev = curr;
        curr->connect_next = fh;

//        qDebug()<<fh->f->path.c_str()<<fh->connect_prev<<fh->connect_next<<"this:"<<fh<<"ms index:"<<fh->multisending_index;
//        qDebug()<<curr->f->path.c_str()<<curr->connect_prev<<curr->connect_next<<"this:"<<curr<<curr->multisending_index;


        files_queue.push(fh);
    }
    void push_file(int file_index)
    {
        file_send_handler* fh = alloc_send_handler();
        fh->f = local_table[file_index];
        fh->index = file_index;
        files_queue.push(fh);
    }
    void push_table()
    {
        if(local_table.size())
        {
            specific_data_send_handler* sh;
            set_mem(sh, 1);
            sh->type = Table;

            FILE_TABLE *local_table_copy = new FILE_TABLE(local_table);
            sh->data = local_table_copy;

            specific_queue.push(sh);
        }
    }
    void push_table_request()
    {
        specific_data_send_handler* sh;
        set_mem(sh, 1);
        sh->type = TableRequest;
        sh->data = nullptr;
        specific_queue.push(sh);
    }
    void push_file_request(int file_index, const char* path = nullptr)
    {
        if(path) out_table[file_index]->path = path;
        else     out_table[file_index]->path = default_path;
        out_table[file_index]->path.append(out_table[file_index]->name);

        specific_data_send_handler* sh;
        set_mem(sh,1);
        sh->type = FileRequest;
        int* index = get_mem<int>(1);
        *index = file_index;
        sh->data = index;
        specific_queue.push(sh);
    }
    void push_multi_files_request(const DArray<int>& files, const char* path = nullptr)
    {
        for(int i=0;i!=files.size();++i)
        {
            if(path) out_table[files[i]]->path = path;
            else     out_table[files[i]]->path = default_path;
            out_table[files[i]]->path.append(out_table[files[i]]->name);

//            qDebug()<<"full path:"<<out_table[files[i]]->path.c_str()<<"for file:"<<files[i];
        }

        specific_data_send_handler* sh;
        set_mem(sh, 1);
        sh->type = MultiFilesRequest;
        DArray<int>* _files = new DArray<int>(files);
        sh->data = _files;
        specific_queue.push(sh);
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

    void force_file(int file_index)
    {
        if(files_queue.size())
        {
            file_send_handler* fh = new file_send_handler;
            fh->f = local_table[file_index];
            fh->interuptable = true;
            fh->connect_next = nullptr;
            fh->connect_prev = nullptr;
            fh->pack_freq = 1;
            fh->multisending_index = last_ms_file++;
            fh->initial_sent = false;
            fh->index = file_index;
            file_send_handler* curr = files_queue.front();
            file_send_handler* next = curr->connect_next;

            if(next)
            {
                curr->connect_next->connect_prev = fh;
                fh->connect_next = curr->connect_next;
                curr->connect_next = fh;
            }
            else
            {
                fh->connect_next = curr;
                curr->connect_next = fh;
                curr->connect_prev = fh;
            }
            fh->connect_prev = curr;
        }
    }
    static void stream(FileMover* fm)
    {
        uint8_t nd_byte = FM_NEW_DATA_BYTE;
        uint8_t ds_byte = FM_DISCONNECT_BYTE;
        //----recv
        uint8_t main_byte = 0;
        int rb = 0;
        header_type rtype = NoType;
        DArray<int> files;

        int ms_files = 0;
        int ms_size = 0;
        int table_size = 0;
        int table_item_packet = 0;
        int table_packets_recv = 0;
        int packet_size = 0;
        file_recv_handler* current_recv_file = nullptr;

        //----send
        header_type ft = File;
        int speed_rate = 4;
        int sb = 0;
        file_send_handler* current_send_file = nullptr;

        int file_byte_sent = 0;

        DArray<send_item>::const_iterator send_progress = nullptr;
        DArray<send_item>::const_iterator send_progress_end = nullptr;

        DArray<send_item> send_content;
        //-----------------
        while(true)
        {
            if(!fm->connection_status) break;
            if(fm->control.check_readability(0,3000))
            {
                if(main_byte)
                {
                    if(main_byte == FM_NEW_DATA_BYTE)
                    {
                        if(rtype == NoType)
                        {
                            rb = fm->control.receive_to(&rtype, sizeof(header_type));
                        }
                        switch(rtype)
                        {
                        case Table:
                        {
                            fm->mu.lock();
                            if(!table_size)
                            {
                                fm->out_table.clear();
                                rb = fm->control.receive_to(&table_size, sizeof(int));
                            }
                            else
                            {
                                rb = fm->control.unlocked_recv_packet(fm->string_buffer.data, &packet_size);
                            }

                            if(packet_size && rb == packet_size)
                            {
                                if(table_item_packet)
                                {
                                    fm->out_table.back()->shortSize = fm->string_buffer.data;
                                    --table_item_packet;
                                    ++table_packets_recv;
                                    packet_size = 0;
                                }
                                else
                                {
                                    file_info* item = new file_info;
                                    item->name = fm->string_buffer.data;
                                    fm->out_table.push_back(item);
                                    ++table_item_packet;
                                    ++table_packets_recv;
                                    packet_size = 0;
                                }
                            }
                            if(table_size && table_packets_recv == table_size)
                            {
                                main_byte = 0;
                                rtype = NoType;
                                packet_size = 0;
                                table_packets_recv = 0;
                                table_item_packet = 0;
                                table_size = 0;
                                fm->print_out_table();
                            }
                            fm->mu.unlock();
                            break;
                        }
                        case Message:
                        {
                            rb = fm->control.unlocked_recv_packet(fm->string_buffer.data, &packet_size);
                            if(packet_size && rb == packet_size)
                            {
                                main_byte = 0;
                                rtype = NoType;
                                packet_size = 0;
                                std::cout << fm->string_buffer.data << std::endl;
                            }
                            break;
                        }
                        case File:
                        {

                            file_recv_handler* h = fm->alloc_recv_handler();
                            while( fm->control.receive_to(&h->index, sizeof(int)) <= 0 ){}
                            h->f = fm->out_table[h->index];
                            while( fm->control.receive_to(&h->f->size, sizeof(size_t)) <= 0 ){}
                            while( fm->control.receive_to(&h->packet_size, sizeof(int)) <= 0 ){}
                            while( fm->control.receive_to(&h->multisending_index, 1) <= 0 ){}

                            qDebug()<<"receive file header"<<"index:"<<h->index<<"size:"<<h->f->size<<"ms index:"<<h->multisending_index<<
                                      "path:"<<h->f->path.c_str()<<"out table size:"<<fm->out_table.size();

                            gettimeofday(&h->start, nullptr);
                            h->f->file = fopen(h->f->path.c_str(), "wb");



                            h->read = 0;
                            h->prc = 0.0;
                            h->bytes_left = h->f->size;

                            h->read_for_packet = 0;
                            h->read_now = (size_t)h->packet_size < fm->recv_buffer.size ? h->packet_size : fm->recv_buffer.size;
                            fm->recv_files[h->multisending_index] = h;
                            rtype = NoType;
                            main_byte = 0;
                            break;
                        }
                        case FileRequest:
                        {
                            int index = 0;
                            while ( fm->control.receive_to(&index, sizeof(int)) <= 0 );
                            if(index >=0 && index < (int)fm->local_table.size())
                            {
                                rtype = NoType;
                                main_byte = 0;
                                fm->push_file(index);
                            }
                            else qDebug()<<"Get File Request with wrong index:"<<index;
                            break;
                        }
                        case MultiFilesRequest:
                        {
                            if(!ms_size) while( fm->control.receive_to(&ms_size, sizeof(int)) <= 0 );
                            else
                            {
                                int index = -1;
                                rb = fm->control.receive_to(&index, sizeof(int));
                                if(rb == sizeof(int))
                                {
                                    ++ms_files;
                                    files.push_back(index);
                                }
                                if(ms_files == ms_size)
                                {
                                    rtype = NoType;
                                    main_byte = 0;
                                    ms_size = 0;
                                    ms_files = 0;
                                    fm->push_files(files);
                                    files.clear();
                                    qDebug()<<"get MultiFilesRequest"<<files.size();
                                }
                            }


                            break;
                        }
                        case TableRequest: break;
                        case NoType: break;
                        }
                    }
                    else
                    {
//                        qDebug()<<"try read file";
                        current_recv_file = fm->recv_files[main_byte];
                        rb = fm->control.unlocked_recv_to(fm->recv_buffer.data, current_recv_file->read_now);
                        if(rb == current_recv_file->read_now)
                        {

                            fwrite(fm->recv_buffer.data, rb, 1, current_recv_file->f->file);
                            if( current_recv_file->bytes_left -= rb )
                            {
                                current_recv_file->read_for_packet += rb;
                                current_recv_file->prc = 100.0 - ((double) current_recv_file->bytes_left * 100.0 / current_recv_file->f->size);
                                current_recv_file->read_now = current_recv_file->bytes_left < (size_t)fm->recv_buffer.size ?
                                                              current_recv_file->bytes_left : fm->recv_buffer.size;
                                if(current_recv_file->read_for_packet == current_recv_file->packet_size)
                                {
                                    current_recv_file->read_for_packet = 0;
                                    main_byte = 0;
                                    rtype = NoType;
                                }
                            }
                            else
                            {

                                timeval end;
                                gettimeofday(&end, nullptr);
                                timeval t = PROFILER::time_dif(&current_recv_file->start, &end);
                                qDebug()<<"recv file:"<<current_recv_file->multisending_index<<current_recv_file->f->path.c_str()
                                       <<current_recv_file->f
                                       <<current_recv_file->f->path.size()
                                       <<"time:"<<t.tv_sec<<"sec"<<t.tv_usec<<"usec";

                                fclose(current_recv_file->f->file);
                                fm->recv_files.erase(fm->recv_files.find(main_byte));
                                fm->out_table.remove(current_recv_file->f);
                                delete current_recv_file->f;
                                delete current_recv_file;
                                current_recv_file = nullptr;
                                main_byte = 0;
                            }


//                            qDebug()<<main_byte<<"ms byte:"<<current_recv_file->multisending_index
//                                   <<"path:"<<current_recv_file->f->path.c_str();


                        }


                    }
                }
                else
                {
                    while( (rb = fm->control.unlocked_recv_to(&main_byte, 1)) <= 0 )
                    {
                        if(rb < 0) break;
                    }
                    if(rb < 0) break;
//                    qDebug()<<"read main byte:"<<main_byte;
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
                    specific_data_send_handler* h = fm->specific_queue.front();
                    switch (h->type)
                    {
                    case Table:
                    {
                        if(!send_progress)
                        {
                            FILE_TABLE* t = (FILE_TABLE*)h->data;
                            int t_size = t->size() * 2;
                            send_content.clear();
                            send_content.push_back({&nd_byte, 1, false});
                            send_content.push_back({&h->type, sizeof(header_type), false});
                            send_content.push_back({&t_size, sizeof(int), false});
                            for(int i=0;i!=t->size();++i)
                            {
                                file_info* it = t->at(i);
                                send_content.push_back({it->name.c_str(), (int)it->name.size() + 1, true});
                                send_content.push_back({it->shortSize.c_str(), (int)it->shortSize.size() + 1, true});
                            }
                            send_progress = send_content.constBegin();
                            send_progress_end = send_content.constEnd();
                        }
                        while( send_progress != send_progress_end && sb >= 0 )
                        {
                            if((*send_progress).packet) sb = fm->control.unlocked_send_packet((*send_progress).data, (*send_progress).size);
                            else sb = fm->control.unlocked_send_it((*send_progress).data, (*send_progress).size);
                            if(sb == (*send_progress).size) ++send_progress;
                        }
                        if(send_progress == send_progress_end)
                        {
                            send_content.clear();
                            fm->specific_queue.pop();
                            send_progress = nullptr;
                            send_progress_end = nullptr;
                            delete (FILE_TABLE*)h->data;
                            free_mem(h);
                        }
                        break;
                    }
                    case TableRequest:
                    {
                        break;
                    }
                    case FileRequest:
                    {
                        fm->control.send_it(&nd_byte, 1);
                        fm->control.send_it(&h->type, sizeof(header_type));
                        fm->control.send_it(h->data, sizeof(int));
                        fm->specific_queue.pop();
                        free_mem(h->data);
                        free_mem(h);
                        break;
                    }
                    case Message:
                    {
                        if(!send_progress)
                        {
                            send_content.clear();
                            send_content.push_back({&nd_byte, 1, false});
                            send_content.push_back({&h->type, sizeof(header_type), false});
                            message_data* md = (message_data*)h->data;
                            send_content.push_back({fm->message_buffer.data + md->begin, md->size, true});

                            send_progress = send_content.constBegin();
                            send_progress_end = send_content.constEnd();
                        }
                        while( send_progress != send_progress_end && sb >= 0 )
                        {
                            if((*send_progress).packet) sb = fm->control.unlocked_send_packet((*send_progress).data, (*send_progress).size);
                            else sb = fm->control.unlocked_send_it((*send_progress).data, (*send_progress).size);
                            if(sb == (*send_progress).size) ++send_progress;
                        }
                        if(send_progress == send_progress_end)
                        {
                            send_content.clear();
                            fm->specific_queue.pop();
                            send_progress = nullptr;
                            send_progress_end = nullptr;
                            free_mem(h->data);
                            free_mem(h);
                        }

                        break;
                    }
                    case MultiFilesRequest:
                    {
                        fm->control.send_it(&nd_byte,1);
                        fm->control.send_it(&h->type, sizeof(header_type));
                        DArray<int>* files = (DArray<int>*)h->data;
                        int size = files->size();
                        fm->control.send_it(&size, sizeof(int));
                        for(int i=0;i!=files->size();++i)
                        {
                            int index = (*files)[i];
                            fm->control.send_it(&index, sizeof(int));
                        }
                        delete files;
                        free_mem(h);
                        fm->specific_queue.pop();
                        qDebug()<<"sent MultiFilesRequest";
                        break;
                    }

                    case File: break;
                    case NoType: break;
                    }

                }
                if(fm->files_queue.size())
                {
//                    qDebug()<<"try send file";
                    if(!current_send_file)
                    {
//                        qDebug()<<"--- 4";
                        current_send_file = fm->files_queue.front();
                    }
                    if(!current_send_file->bytes_left)
                    {
                        send_content.clear();
                        send_content.push_back({&nd_byte, 1, false});
                        send_content.push_back({&ft, sizeof(header_type), false});
                        //file header:
                        current_send_file->interuptable = false;
                        current_send_file->initial_sent = false;
                        current_send_file->bytes_left = current_send_file->f->size;
                        current_send_file->flush_now = current_send_file->bytes_left < (size_t)fm->send_buffer.size ?
                                                       current_send_file->bytes_left : fm->send_buffer.size;
                        current_send_file->prc = 0.0;
                        current_send_file->pack_sent = 0;


                        send_content.push_back({&current_send_file->index, sizeof(int), false}); //index
                        send_content.push_back({&current_send_file->bytes_left, sizeof(size_t), false}); // file size
                        send_content.push_back({&current_send_file->flush_now, sizeof(int), false}); // packet size
                        send_content.push_back({&current_send_file->multisending_index, 1, false}); // ms index


                        send_progress = send_content.constBegin();
                        send_progress_end = send_content.constEnd();
                        gettimeofday(&current_send_file->start, nullptr);

//                        qDebug()<<"--- 3 INITIAL FOR FILE:"<<current_send_file->multisending_index;
                        sb = 0;
                    }

                    while( send_progress != send_progress_end )
                    {
                        header_type type = *(header_type*)(*send_progress).data;
                        sb = fm->control.unlocked_send_it((*send_progress).data, (*send_progress).size);
                        if(sb == (*send_progress).size) ++send_progress;
                        else if(sb < 0) break;
//                        qDebug()<<"--- 2"<<"sb:"<<sb<<type<<current_send_file->multisending_index;
                    }
                    if( send_progress == send_progress_end && !current_send_file->initial_sent)
                    {
//                        qDebug()<<"--- 1";
                        current_send_file->interuptable = true;
                        current_send_file->initial_sent = true;
                        send_progress = nullptr;
                        send_progress_end = nullptr;
                        gettimeofday(&current_send_file->last, nullptr);
                    }
                    if( current_send_file->initial_sent )
                    {
                        if(!file_byte_sent)
                        {
//                            qDebug()<<"send file byte"<<current_send_file->multisending_index;
                            file_byte_sent = fm->control.send_it(&current_send_file->multisending_index, 1);
                        }
                        if(file_byte_sent == 1)
                        {
//                            qDebug()<<"---- write file (initial sent)"<<current_send_file->multisending_index;
                            if(current_send_file->interuptable) fread(fm->send_buffer.data, current_send_file->flush_now, 1, current_send_file->f->file);
                            current_send_file->interuptable = false;
                            sb = fm->control.unlocked_send_it(fm->send_buffer.data, current_send_file->flush_now);

                            qDebug()<<"send part of file:"<<current_send_file->multisending_index<<current_send_file->f->path.c_str()
                                   <<"csf:"<<current_send_file;
//                                   <<"file info:"<<current_send_file->f
//                                   <<"file ptr:"<<current_send_file->f->file
//                                   <<"sb:"<<sb
//                                   <<"flush now:"<<current_send_file->flush_now;

                            if(sb == current_send_file->flush_now)
                            {
//                                qDebug()<<"--- 5";
                                file_byte_sent = 0;
                                if( current_send_file->bytes_left -= sb)
                                {
                                    current_send_file->flush_now = current_send_file->bytes_left < (size_t)fm->send_buffer.size ?
                                                                   current_send_file->bytes_left : fm->send_buffer.size;

                                    current_send_file->interuptable = true;
                                    current_send_file->prc = 100.0 - ((double) current_send_file->bytes_left * 100.0 / current_send_file->f->size);
                                    if(current_send_file->connect_next && ++current_send_file->pack_sent == current_send_file->pack_freq)
                                    {
                                        current_send_file->pack_sent = 0;
                                        current_send_file = current_send_file->connect_next;
                                    }
                                }
                                else
                                {
//                                    qDebug()<<"--- 6";
                                    timeval end;
                                    gettimeofday(&end, nullptr);
                                    timeval t = PROFILER::time_dif(&current_send_file->start, &end);
                                    qDebug()<<"sent file:"<<current_send_file->f->name.c_str()<<"time:"<<t.tv_sec<<"sec"<<t.tv_usec<<"usec";

                                    fclose(current_send_file->f->file);
                                    fm->local_table.remove(current_send_file->f);
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
                                        fm->files_queue.front() = current_send_file->connect_next;
                                    }
                                    else
                                    {
                                        fm->files_queue.pop();
                                    }
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
                        else
                        {
                            current_send_file->interuptable = true;
                        }
                    }
                }
            }
            else
            {
//                qDebug()<<"Can't write";
            }
            if(main_byte == FM_DISCONNECT_BYTE) break;
//            if(current_recv_file) qDebug()<<"recv progress:"<<"file:"<<current_recv_file->multisending_index<<current_recv_file->prc<<'%';
//            if(current_send_file) qDebug()<<"send progress:"<<"file:"<<current_send_file->multisending_index<<current_send_file->prc<<'%'<<current_send_file->speed;
        }


        qDebug()<<"Main Stream Is Over";
    }
public:
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


        if(strcmp(cmd, "info") == 0)
        {
            qDebug()<<"local table size:"<<local_table.size();
            qDebug()<<"out table size:"<<out_table.size();
            qDebug()<<"files queue size:"<<files_queue.size();
            qDebug()<<"recv files size:"<<recv_files.size();
            qDebug()<<"spec queue size:"<<specific_queue.size();
        }
        if(strcmp(cmd, "test") == 0)
        {
            prepare_to_send("G://DH//e1.mkv");
            prepare_to_send("G://DH//sv_test.mkv");
            prepare_to_send("G://DH//e2.mkv");
            prepare_to_send("G://DH//t1_.txt");
            prepare_to_send("G://DH//pic_car.jpg");
            prepare_to_send("G://DH//ost.zip");
            push_table();

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
                else qDebug()<<"Open Connection Error: Wrong port number:"<<port;
            }
            else qDebug()<<"Open Connection Error: No Value";
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
                else qDebug()<<"Connect To Error: Wrong Port Number:"<<port;
            }
            else qDebug()<<"Connect To Error: No Value";
        }
        if(strcmp(cmd, "get files") == 0)
        {
            if(value)
            {
                DArray<int> list;
                const char* path = nullptr;
                int v = -1;
                while(*value != '\0')
                {
                    if(v < 0 && *value != ' ')
                    {
                        v = atoi(value);
                        if(v>=0 && v < out_table.size()) list.push_back(v);
                        else qDebug()<<"Get Files Error: Wrong Index:"<<v;
                    }
                    if(v >= 0 && *value == ',') v = -1;
                    ++value;
                    if(v >= 0 && *value != ' ' && *value != ',' && *value != '\0' && !path) path = value;
                }
                auto b = list.constBegin();
                auto e = list.constEnd();
                qDebug()<<"list:";
                while( b != e)
                {
                    qDebug()<<*b++;
                }
                qDebug()<<"path:"<<path<<(void*)path;


                push_multi_files_request(list, path);
            }
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
            else qDebug()<<"Set Default Path Error: No Value";
        }
        if(strcmp(cmd, "list")==0)
        {
            if(value)
            {
                DArray<int> list;
                const char* index = nullptr;
                list.push_back(atoi(value));
                while(*value != '\0')
                {
                    if(!index && *value == ',') index = value;
                    ++value;
                    if(index && *value != ' ')
                    {
                        int v = atoi(value);
                        if(v > 0 && v < (int)out_table.size())
                            list.push_back(v);
                        else
                        {
                            qDebug()<<"Wrong index:"<<v;
                            break;
                        }
                        index = nullptr;
                    }
                }
            }
        }
        if(strcmp(cmd, "load file") == 0)
        {
            if(value)
            {
                prepare_to_send(value);
                push_table();
            }
            else qDebug()<<"No value";
        }
        if(strcmp(cmd, "otable") == 0)
        {
            print_out_table();
        }
        if(strcmp(cmd, "ltable") == 0)
        {
            print_local_table();
        }
        if(strcmp(cmd, "get file") == 0)
        {
            if(out_table.empty())
            {
                qDebug()<<"files table is emty!"; return;
            }
            if(value)
            {
                int index = atoi(value);
                const char* path = nullptr;
                while(*value != '\0')
                {
                    if(!path && *value == ' ') path = value;
                    if(path  && *value != ' ') {path = value; break;}
                    ++value;
                }
                if(index >=0 && index < (int)out_table.size())
                {
                    push_file_request(index, path);
                }
                else qDebug()<<"get file: wrong index:"<<index;
            }
            else qDebug()<<"no value";
        }
        if(strcmp(cmd, "force file") == 0)
        {
            if(value)
            {
                int index = atoi(value);
                if(index >=0 && index < (int)out_table.size()) force_file(index);
                else qDebug()<<"Force File Error: Wrong Index:"<<index;
            }
            else qDebug()<<"Force File Error: NoValue";
        }
        if(strcmp(cmd, "send table") == 0)
        {
            push_table();
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
    }
    void prepare_to_send(const char* path)
    {
        FILE* file = fopen(path, "rb");
        if(!file)
        {
            qDebug()<<"Load file: wrong path";
            return;
        }
        const char* pi = path;
        const char* pn = path;
        while(*pi != '\0')
        {
            if(*pi == '/') pn = pi+1;
            ++pi;
        }
        std::ifstream f(path, std::ios::in | std::ios::binary);
        f.seekg(0, std::ios::end);
        size_t bytes = f.tellg();
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
        file_info* fi = new file_info;
        fi->path = path;
        fi->name = pn;
        fi->shortSize = shortSize;
        fi->size = size;
        fi->file = file;
        print_file_info(fi);
        local_table.push_back(fi);
    }
//v 1.5.0
};



/*
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

        set_default_download_path(FM_DEFAULT_DOWNLOAD_PATH);
    }
    typedef std::string string;
//private:
public:
    DTcp control;
    bool connection_status;

    const int system_index_shift = 10;
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
        size_t size;
        std::vector<file_info*>::const_iterator place;
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
        int index;
        double prc;
        struct timeval start;
        size_t bytes_left;
        int flush_now;

        char speed[10];
        struct timeval last;

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
        return h;
    }
    struct specific_data_send_handler
    {
        header_type type;
        void* data;
    };



    std::map<int, file_send_handler*>        send_map;
    std::deque<file_send_handler*>           send_queue;

    std::map<int, file_recv_handler*>        recv_map;
    std::queue<int> available_index;


    std::queue<specific_data_send_handler*>      specific_queue;

    struct send_item
    {
        const void* data;
        int size;
        bool packet;
    };
    struct message_data
    {
        int begin;
        int size;
    };


    void print_file_info(const file_info* fi) const
    {
        std::cout << "File: " << fi->path << " " << fi->shortSize << std::endl;
    }
    void print_local_table() const
    {
        if(!send_map.size())
        {
            qDebug()<<"Local table is empty";
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
            qDebug()<<"Out table is empty";
            return;
        }
        auto b = recv_map.cbegin();
        auto e = recv_map.cend();
        std::cout << "Out Table:" << std::endl;
        while( b!=e )
        {
            std::cout << b->first <<" : ";
            print_file_info(b->second->f);
            ++b;
        }
    }
    void print_send_queue() const
    {
        if(!send_queue.size())
        {
            qDebug()<<"Send Queue is empty";
            return;
        }
        auto b = send_queue.begin();
        auto e = send_queue.end();
        while( b != e ) print_file_info((*b++)->f);
    }

    void push_files(const DArray<int>& files)
    {
        file_send_handler* first = send_map[files.constFront()];
        file_send_handler* last  = first;
        auto b = files.constBegin() + 1;
        auto e = files.constEnd();
        while( b != e )
        {
            auto curr = send_map[*b];
            last->connect_next = curr;
            curr->connect_prev = last;
            curr = last;
            ++b;
        }
        last->connect_next = first;
        first->connect_prev = last;
        send_queue.push_back(first);
    }
    void push_file(int file_index)
    {
        send_queue.push_back(send_map[file_index]);
    }
    void push_new_file_handler(file_info* f)
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
    void push_table_request()
    {
        specific_data_send_handler* sh;
        set_mem(sh, 1);
        sh->type = TableRequest;
        sh->data = nullptr;
        specific_queue.push(sh);
    }
    void push_file_request(int file_index, const char* path = nullptr)
    {
        recv_map[file_index]->f->path = path ? path : default_path;
        recv_map[file_index]->f->path.append(recv_map[file_index]->f->name);

        specific_data_send_handler* sh;
        set_mem(sh,1);
        sh->type = FileRequest;
        int* index = get_mem<int>(1);
        *index = file_index;
        sh->data = index;
        specific_queue.push(sh);
    }
    void push_multi_files_request(const DArray<int>& files, const char* path = nullptr)
    {
        for(int i=0;i!=files.size();++i)
        {
            recv_map[files[i]]->f->path = path ? path : default_path;
            recv_map[files[i]]->f->path.append(recv_map[files[i]]->f->name);
        }
        specific_data_send_handler* sh;
        set_mem(sh, 1);
        sh->type = MultiFilesRequest;
        DArray<int>* _files = new DArray<int>(files);
        sh->data = _files;
        specific_queue.push(sh);
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
    void force_file(int file_index)
    {
//        if(files_queue.size())
//        {
//            file_send_handler* fh = new file_send_handler;
//            fh->f = local_table[file_index];
//            fh->interuptable = true;
//            fh->connect_next = nullptr;
//            fh->connect_prev = nullptr;
//            fh->pack_freq = 1;
//            fh->multisending_index = last_ms_file++;
//            fh->initial_sent = false;
//            fh->index = file_index;
//            file_send_handler* curr = files_queue.front();
//            file_send_handler* next = curr->connect_next;

//            if(next)
//            {
//                curr->connect_next->connect_prev = fh;
//                fh->connect_next = curr->connect_next;
//                curr->connect_next = fh;
//            }
//            else
//            {
//                fh->connect_next = curr;
//                curr->connect_next = fh;
//                curr->connect_prev = fh;
//            }
//            fh->connect_prev = curr;
//        }
    }
    static void stream(FileMover* fm)
    {
        uint8_t nd_byte = FM_NEW_DATA_BYTE;
        uint8_t ds_byte = FM_DISCONNECT_BYTE;
        //----recv
        uint8_t main_byte = 0;
        int rb = 0;
        header_type rtype = NoType;
        DArray<int> files;

        int ms_files = 0;
        int ms_size = 0;
        int table_size = 0;
        int table_item_packet = 0;
        int table_packets_recv = 0;
        int packet_size = 0;
        file_recv_handler* current_recv_file = nullptr;

        //----send
        header_type ft = File;
        int speed_rate = 4;
        int sb = 0;
        file_send_handler* current_send_file = nullptr;

        int file_byte_sent = 0;

        DArray<send_item>::const_iterator send_progress = nullptr;
        DArray<send_item>::const_iterator send_progress_end = nullptr;

        DArray<send_item> send_content;
        //-----------------
        while(true)
        {
            if(!fm->connection_status) break;
            if(fm->control.check_readability(0,3000))
            {
                if(main_byte)
                {
                    if(main_byte == FM_NEW_DATA_BYTE)
                    {
                        if(rtype == NoType)
                        {
                            rb = fm->control.receive_to(&rtype, sizeof(header_type));
                            qDebug()<<"try read rtype:"<<rtype<<rb;
                        }
//                        qDebug()<<"try read new data. type:"<<rtype;
                        switch(rtype)
                        {
                        case NewFileInSystem:
                        {

                            // ------------------------------- ?????????????
                            break;
                        }
                        case Message:
                        {
                            rb = fm->control.unlocked_recv_packet(fm->string_buffer.data, &packet_size);
                            if(packet_size && rb == packet_size)
                            {
                                main_byte = 0;
                                rtype = NoType;
                                packet_size = 0;
                                std::cout << fm->string_buffer.data << std::endl;
                            }
                            break;
                        }
                        case File:
                        {
                            file_recv_handler* h = fm->alloc_recv_handler();
                            while( fm->control.receive_to(&h->index, sizeof(int)) <= 0 ){}
                            fm->recv_map[h->index] = h;
                            while( fm->control.receive_to(&h->f->size, sizeof(size_t)) <= 0 ){}
                            while( fm->control.receive_to(&h->packet_size, sizeof(int)) <= 0 ){}

                            qDebug()<<"receive file header"<<"index:"<<h->index<<"size:"<<h->f->size<<"ms index:"<<h->index<<
                                      "path:"<<h->f->path.c_str();

                            gettimeofday(&h->start, nullptr);
                            h->f->file = fopen(h->f->path.c_str(), "wb");



                            h->read = 0;
                            h->prc = 0.0;
                            h->bytes_left = h->f->size;

                            h->read_for_packet = 0;
                            h->read_now = (size_t)h->packet_size < fm->recv_buffer.size ? h->packet_size : fm->recv_buffer.size;
                            rtype = NoType;
                            main_byte = 0;
                            break;
                        }
                        case FileRequest:
                        {
                            int index = 0;
                            while ( fm->control.receive_to(&index, sizeof(int)) <= 0 );
                            rtype = NoType;
                            main_byte = 0;
                            fm->push_file(index);
                            break;
                        }
                        case MultiFilesRequest:
                        {
                            if(!ms_size) while( fm->control.receive_to(&ms_size, sizeof(int)) <= 0 );
                            else
                            {
                                int index = -1;
                                rb = fm->control.receive_to(&index, sizeof(int));
                                if(rb == sizeof(int))
                                {
                                    ++ms_files;
                                    files.push_back(index);
                                }
                                if(ms_files == ms_size)
                                {
                                    rtype = NoType;
                                    main_byte = 0;
                                    ms_size = 0;
                                    ms_files = 0;
                                    fm->push_files(files);
                                    files.clear();
                                    qDebug()<<"get MultiFilesRequest"<<files.size();
                                }
                            }


                            break;
                        }
                        case TableRequest: break;
                        case NoType: break;
                        }
                    }
                    else
                    {
                        current_recv_file = fm->recv_map[main_byte];
                        rb = fm->control.unlocked_recv_to(fm->recv_buffer.data, current_recv_file->read_now);
                        if(rb == current_recv_file->read_now)
                        {

                            fwrite(fm->recv_buffer.data, rb, 1, current_recv_file->f->file);
                            if( current_recv_file->bytes_left -= rb )
                            {
                                current_recv_file->read_for_packet += rb;
                                current_recv_file->prc = 100.0 - ((double) current_recv_file->bytes_left * 100.0 / current_recv_file->f->size);
                                current_recv_file->read_now = current_recv_file->bytes_left < (size_t)fm->recv_buffer.size ?
                                                              current_recv_file->bytes_left : fm->recv_buffer.size;
                                if(current_recv_file->read_for_packet == current_recv_file->packet_size)
                                {
                                    current_recv_file->read_for_packet = 0;
                                    main_byte = 0;
                                    rtype = NoType;
                                }
                            }
                            else
                            {

                                timeval end;
                                gettimeofday(&end, nullptr);
                                timeval t = PROFILER::time_dif(&current_recv_file->start, &end);
                                qDebug()<<"recv file:"<<current_recv_file->index<<current_recv_file->f->path.c_str()
                                       <<current_recv_file->f
                                       <<current_recv_file->f->path.size()
                                       <<"time:"<<t.tv_sec<<"sec"<<t.tv_usec<<"usec";

                                fclose(current_recv_file->f->file);
//                                fm->recv_files.erase(fm->recv_files.find(main_byte));
                                delete current_recv_file->f;
                                delete current_recv_file;
                                current_recv_file = nullptr;
                                main_byte = 0;
                            }


//                            qDebug()<<main_byte<<"ms byte:"<<current_recv_file->multisending_index
//                                   <<"path:"<<current_recv_file->f->path.c_str();


                        }


                    }
                }
                else
                {
                    while( (rb = fm->control.unlocked_recv_to(&main_byte, 1)) <= 0 )
                    {
                        if(rb < 0) break;
                    }
                    if(rb < 0) break;
//                    qDebug()<<"read main byte:"<<main_byte;
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
                    specific_data_send_handler* h = fm->specific_queue.front();
                    switch (h->type)
                    {
                    case NewFileInSystem:
                    {
                        break;
                    }
                    case TableRequest:
                    {
                        break;
                    }
                    case FileRequest:
                    {
                        fm->control.send_it(&nd_byte, 1);
                        fm->control.send_it(&h->type, sizeof(header_type));
                        fm->control.send_it(h->data, sizeof(int));
                        fm->specific_queue.pop();
                        free_mem(h->data);
                        free_mem(h);
                        break;
                    }
                    case Message:
                    {
                        if(!send_progress)
                        {
                            send_content.clear();
                            send_content.push_back({&nd_byte, 1, false});
                            send_content.push_back({&h->type, sizeof(header_type), false});
                            message_data* md = (message_data*)h->data;
                            send_content.push_back({fm->message_buffer.data + md->begin, md->size, true});

                            send_progress = send_content.constBegin();
                            send_progress_end = send_content.constEnd();
                        }
                        while( send_progress != send_progress_end && sb >= 0 )
                        {
                            if((*send_progress).packet) sb = fm->control.unlocked_send_packet((*send_progress).data, (*send_progress).size);
                            else sb = fm->control.unlocked_send_it((*send_progress).data, (*send_progress).size);
                            if(sb == (*send_progress).size) ++send_progress;
                        }
                        if(send_progress == send_progress_end)
                        {
                            send_content.clear();
                            fm->specific_queue.pop();
                            send_progress = nullptr;
                            send_progress_end = nullptr;
                            free_mem(h->data);
                            free_mem(h);
                        }

                        break;
                    }
                    case MultiFilesRequest:
                    {
                        fm->control.send_it(&nd_byte,1);
                        fm->control.send_it(&h->type, sizeof(header_type));
                        DArray<int>* files = (DArray<int>*)h->data;
                        int size = files->size();
                        fm->control.send_it(&size, sizeof(int));
                        for(int i=0;i!=files->size();++i)
                        {
                            int index = (*files)[i];
                            fm->control.send_it(&index, sizeof(int));
                        }
                        delete files;
                        free_mem(h);
                        fm->specific_queue.pop();
                        qDebug()<<"sent MultiFilesRequest";
                        break;
                    }

                    case File: break;
                    case NoType: break;
                    }

                }
                if(!fm->send_queue.empty())
                {
                    if(!current_send_file)
                    {
                        current_send_file = fm->send_queue.front();
                    }
                    if(!current_send_file->bytes_left)
                    {
                        send_content.clear();
                        send_content.push_back({&nd_byte, 1, false});
                        send_content.push_back({&ft, sizeof(header_type), false});
                        //file header:
                        current_send_file->interuptable = false;
                        current_send_file->initial_sent = false;
                        current_send_file->bytes_left = current_send_file->f->size;
                        current_send_file->flush_now = current_send_file->bytes_left < (size_t)fm->send_buffer.size ?
                                                       current_send_file->bytes_left : fm->send_buffer.size;
                        current_send_file->prc = 0.0;
                        current_send_file->pack_sent = 0;


                        send_content.push_back({&current_send_file->index, sizeof(int), false}); //index
                        send_content.push_back({&current_send_file->bytes_left, sizeof(size_t), false}); // file size
                        send_content.push_back({&current_send_file->flush_now, sizeof(int), false}); // packet size


                        send_progress = send_content.constBegin();
                        send_progress_end = send_content.constEnd();
                        gettimeofday(&current_send_file->start, nullptr);

//                        qDebug()<<"--- 3 INITIAL FOR FILE:"<<current_send_file->multisending_index;
                    }

                    while( send_progress != send_progress_end )
                    {
                        header_type type = *(header_type*)(*send_progress).data;
                        sb = fm->control.unlocked_send_it((*send_progress).data, (*send_progress).size);
                        if(sb == (*send_progress).size) ++send_progress;
                        else if(sb < 0) break;
//                        qDebug()<<"--- 2"<<"sb:"<<sb<<type<<current_send_file->multisending_index;
                    }
                    if( send_progress == send_progress_end && !current_send_file->initial_sent)
                    {
//                        qDebug()<<"--- 1";
                        current_send_file->interuptable = true;
                        current_send_file->initial_sent = true;
                        send_progress = nullptr;
                        send_progress_end = nullptr;
                        gettimeofday(&current_send_file->last, nullptr);
                    }
                    if( current_send_file->initial_sent )
                    {
                        if(!file_byte_sent)
                        {
//                            qDebug()<<"send file byte"<<current_send_file->multisending_index;
                            file_byte_sent = fm->control.send_it(&current_send_file->index, 1);
                        }
                        if(file_byte_sent == 1)
                        {
//                            qDebug()<<"---- write file (initial sent)"<<current_send_file->multisending_index;
                            if(current_send_file->interuptable) fread(fm->send_buffer.data, current_send_file->flush_now, 1, current_send_file->f->file);
                            current_send_file->interuptable = false;
                            sb = fm->control.unlocked_send_it(fm->send_buffer.data, current_send_file->flush_now);

                            qDebug()<<"send part of file:"<<current_send_file->index<<current_send_file->f->path.c_str()
                                   <<"csf:"<<current_send_file;
//                                   <<"file info:"<<current_send_file->f
//                                   <<"file ptr:"<<current_send_file->f->file
//                                   <<"sb:"<<sb
//                                   <<"flush now:"<<current_send_file->flush_now;

                            if(sb == current_send_file->flush_now)
                            {
//                                qDebug()<<"--- 5";
                                file_byte_sent = 0;
                                if( current_send_file->bytes_left -= sb)
                                {
                                    current_send_file->flush_now = current_send_file->bytes_left < (size_t)fm->send_buffer.size ?
                                                                   current_send_file->bytes_left : fm->send_buffer.size;

                                    current_send_file->interuptable = true;
                                    current_send_file->prc = 100.0 - ((double) current_send_file->bytes_left * 100.0 / current_send_file->f->size);
                                    if(current_send_file->connect_next && ++current_send_file->pack_sent == current_send_file->pack_freq)
                                    {
                                        current_send_file->pack_sent = 0;
                                        current_send_file = current_send_file->connect_next;
                                    }
                                }
                                else
                                {
//                                    qDebug()<<"--- 6";
                                    timeval end;
                                    gettimeofday(&end, nullptr);
                                    timeval t = PROFILER::time_dif(&current_send_file->start, &end);
                                    qDebug()<<"sent file:"<<current_send_file->f->name.c_str()<<"time:"<<t.tv_sec<<"sec"<<t.tv_usec<<"usec";

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
                        else
                        {
                            current_send_file->interuptable = true;
                        }
                    }
                }
            }
            else
            {
//                qDebug()<<"Can't write";
            }
            if(main_byte == FM_DISCONNECT_BYTE) break;
//            if(current_recv_file) qDebug()<<"recv progress:"<<"file:"<<current_recv_file->multisending_index<<current_recv_file->prc<<'%';
//            if(current_send_file) qDebug()<<"send progress:"<<"file:"<<current_send_file->multisending_index<<current_send_file->prc<<'%'<<current_send_file->speed;
        }


        qDebug()<<"Main Stream Is Over";
    }
public:
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


        if(strcmp(cmd, "info") == 0)
        {
            qDebug()<<"local table size:"<<local_table.size();
            qDebug()<<"out table size:"<<out_table.size();
            qDebug()<<"files queue size:"<<files_queue.size();
            qDebug()<<"recv files size:"<<recv_files.size();
            qDebug()<<"spec queue size:"<<specific_queue.size();
        }
        if(strcmp(cmd, "test") == 0)
        {
            prepare_to_send("G://DH//e1.mkv");
            prepare_to_send("G://DH//sv_test.mkv");
            prepare_to_send("G://DH//e2.mkv");
            prepare_to_send("G://DH//t1_.txt");
            prepare_to_send("G://DH//pic_car.jpg");
            prepare_to_send("G://DH//ost.zip");
            push_table();

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
                else qDebug()<<"Open Connection Error: Wrong port number:"<<port;
            }
            else qDebug()<<"Open Connection Error: No Value";
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
                else qDebug()<<"Connect To Error: Wrong Port Number:"<<port;
            }
            else qDebug()<<"Connect To Error: No Value";
        }
        if(strcmp(cmd, "get files") == 0)
        {
            if(value)
            {
                DArray<int> list;
                const char* path = nullptr;
                int v = -1;
                while(*value != '\0')
                {
                    if(v < 0 && *value != ' ')
                    {
                        v = atoi(value);
                        if(v>=0 && v < out_table.size()) list.push_back(v);
                        else qDebug()<<"Get Files Error: Wrong Index:"<<v;
                    }
                    if(v >= 0 && *value == ',') v = -1;
                    ++value;
                    if(v >= 0 && *value != ' ' && *value != ',' && *value != '\0' && !path) path = value;
                }
                auto b = list.constBegin();
                auto e = list.constEnd();
                qDebug()<<"list:";
                while( b != e)
                {
                    qDebug()<<*b++;
                }
                qDebug()<<"path:"<<path<<(void*)path;


                push_multi_files_request(list, path);
            }
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
            else qDebug()<<"Set Default Path Error: No Value";
        }
        if(strcmp(cmd, "list")==0)
        {
            if(value)
            {
                DArray<int> list;
                const char* index = nullptr;
                list.push_back(atoi(value));
                while(*value != '\0')
                {
                    if(!index && *value == ',') index = value;
                    ++value;
                    if(index && *value != ' ')
                    {
                        int v = atoi(value);
                        if(v > 0 && v < (int)out_table.size())
                            list.push_back(v);
                        else
                        {
                            qDebug()<<"Wrong index:"<<v;
                            break;
                        }
                        index = nullptr;
                    }
                }
            }
        }
        if(strcmp(cmd, "load file") == 0)
        {
            if(value)
            {
                prepare_to_send(value);
                push_table();
            }
            else qDebug()<<"No value";
        }
        if(strcmp(cmd, "otable") == 0)
        {
            print_out_table();
        }
        if(strcmp(cmd, "ltable") == 0)
        {
            print_local_table();
        }
        if(strcmp(cmd, "get file") == 0)
        {
            if(out_table.empty())
            {
                qDebug()<<"files table is emty!"; return;
            }
            if(value)
            {
                int index = atoi(value);
                const char* path = nullptr;
                while(*value != '\0')
                {
                    if(!path && *value == ' ') path = value;
                    if(path  && *value != ' ') {path = value; break;}
                    ++value;
                }
                if(index >=0 && index < (int)out_table.size())
                {
                    push_file_request(index, path);
                }
                else qDebug()<<"get file: wrong index:"<<index;
            }
            else qDebug()<<"no value";
        }
        if(strcmp(cmd, "force file") == 0)
        {
            if(value)
            {
                int index = atoi(value);
                if(index >=0 && index < (int)out_table.size()) force_file(index);
                else qDebug()<<"Force File Error: Wrong Index:"<<index;
            }
            else qDebug()<<"Force File Error: NoValue";
        }
        if(strcmp(cmd, "send table") == 0)
        {
            push_table();
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
    }
    void prepare_to_send(const char* path)
    {
        FILE* file = fopen(path, "rb");
        if(!file)
        {
            qDebug()<<"Load file: wrong path";
            return;
        }
        const char* pi = path;
        const char* pn = path;
        while(*pi != '\0')
        {
            if(*pi == '/') pn = pi+1;
            ++pi;
        }
        std::ifstream f(path, std::ios::in | std::ios::binary);
        f.seekg(0, std::ios::end);
        size_t bytes = f.tellg();
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
        send_map[h->index] = h;
        print_file_info(h->f);
    }
//v 1.5.0
};
*/

#endif // FILEMOVER_H
