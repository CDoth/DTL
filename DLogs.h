#ifndef DLOGS_H
#define DLOGS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include <cstddef>
#include <iostream>
#include <stddef.h>

// add std::nullptr_t
// -: new line in destructor of global objects
// message counter in DLogContext
// logs file rewrite
// put flags out
namespace DLogs
{
//------------------------------------------------------------------- log context:
#define DLOGM_BUFFER_SIZE 1024
#define DLOGM_BUFFER_MAX_SIZE 1024 * 12
#define DLOGM_FUNC_NAME __func__
#define DLOGM_SEP_SIZE 15
#define DLOGM_SEP_SYMB '-'

enum format_flag
{
    FLUSH_ALL     = 0b00000001,
    FLUSH_MESSAGE = 0b00000010,
    FLUSH_CONTENT     = 0b00000100,
};

    class DLogMaster;
    class DLogContext
    {
    public:
        friend class DLogMaster;
        DLogContext(const char* stream_name = nullptr, const char* log_message = nullptr);
        ~DLogContext();

        //----------parse
        void parse_console(bool _state) {parse_to_console = _state;}
        void parse_file   (bool _state) {parse_to_file    = _state;}

        void parse_console_tool(void (*__parse_c)(const char*))        {c_print = __parse_c;}
        void parse_file_tool   (void (*__parse_f)(FILE*, const char*)) {f_print = __parse_f;}
        //----------file
        void file_add   (const char* path, const char* key);
        void file_lock  (const char* key);
        void file_unlock(const char* key);
        void file_remove(const char* key);
        //----------header resolution
        struct header_resolution
        {
            bool cdate;
            bool fname;
            bool ltime;
            bool mnumb;
            bool rtime;
            bool sname;
        };


        void header_set_all(bool date   = false,
                            bool fname  = false,
                            bool ltime  = false,
                            bool rtime  = false,
                            bool mesnum = false,
                            bool sname  = false);
        void header_set    (const header_resolution& _hr) {hr = _hr;}

        void header_date_visibility  ( bool _state) {hr.cdate = _state;}
        void header_fname_visibility ( bool _state) {hr.fname = _state;}
        void header_ltime_visibility ( bool _state) {hr.ltime = _state;}
        void header_rtime_visibility ( bool _state) {hr.mnumb = _state;}
        void header_mesnum_visibility( bool _state) {hr.rtime = _state;}
        void header_sname_visibility ( bool _state) {hr.sname = _state;}

        void header_set_message(const char* ln);
        //----------separator
        void separator_set_size(int size);
        void separator_set_symb(const char& symb);
        //----------precision
        void precision_float (int p) {float_precision  = p;}
        void precision_double(int p) {double_precision = p;}
        //----------buffer
        void buffer_set_size(int size);


        //----------format
        void format_set_flag(format_flag f) {fflags |= f;}
        void format_disable_flag(format_flag f) {fflags &= ~f;}

    private:
        typedef struct file_descriptor
        {
            FILE* file;
            file_descriptor* next;

            char* path;
            char* key;
            bool is_locked;
        }*pfd;

        pfd first_file;

        bool parse_to_file;
        bool parse_to_console;

        int message_counter;

        int sep_size;
        int sep_symb;

        int float_precision;
        int double_precision;

        int   buffer_size;
        char* buffer;
        char* b_iterator;

        char* stream_name;
        char* header_message;
        char* separator;

        header_resolution hr;
        uint8_t fflags;
    private:
        void start_init();

        void (*c_print)(const char*);
        void (*f_print)(FILE *,const char*);

        void out(const char*);
        void flush(const char* line = nullptr);
        void clear_buffer();
        void new_line();
        void add_header(const char* fname = nullptr);
        bool check_fflag(format_flag);
        //------- buffers
        char* alloc_line(int size);
        char* copy_line(const char* from);
        void  free_line(char* line);
        void  zero_line(char* line, int size = 0);
        //------- file
        pfd alloc_fd();
        void free_fd(pfd);
        void parse_file_info(FILE* f, const char& state);

        void parse_date(char* to = nullptr);
        void parse_ltime(char* to = nullptr);
        void parse_rtime(char* to = nullptr);
    };
    void default_c_print(const char*);
    void default_f_print(FILE*, const char*);

    extern DLogContext gl_raw_context;
    extern DLogContext gl_error_context;
    extern DLogContext gl_info_context;

//---------------------------------------------------------------------------------- log master:
    struct separator_t
    {

    }extern separator;
    class DLogMaster
    {
    public:
        DLogMaster(DLogContext* c = nullptr, bool auto_n = false);
        ~DLogMaster();

        DLogMaster &operator() (const char* _caller_name = nullptr);
        DLogMaster &with_header(const char* _caller_name = nullptr);

        void sep();


        DLogMaster &operator <<(const int&    v);
        DLogMaster &operator <<(const char*   v);
        DLogMaster &operator <<(const float&  v);
        DLogMaster &operator <<(const double& v);
        DLogMaster &operator <<(const bool&   v);
        DLogMaster &operator <<(const long&   v);
        DLogMaster &operator <<(void* p);
        DLogMaster &operator <<(std::nullptr_t);
        DLogMaster &operator <<(DLogMaster&);
        DLogMaster &operator <<(separator_t);


        template <class ... Args>
        void log(const char* fmt, Args ... a)
        {
            snprintf(context->buffer, DLOGM_BUFFER_SIZE ,fmt, a...);
            context->flush();
        }
    private:
        bool auto_n;
        DLogContext* context;
    };

    extern DLogMaster lmraw;
    extern DLogMaster lmerror;
    extern DLogMaster lminfo;

    template <class ... Args>
    void log(const char* fmt, Args ... a) //to raw context
    {
        DLogMaster(&gl_raw_context, true).with_header().log(fmt, a...);
    }

    template <class ... Args>
    void log(DLogContext* c, const char* fmt, Args ... a)
    {
        DLogMaster (c, true).with_header().log(fmt, a...);
    }

    template <class ... Args>
    void log(const char* fname, const char* fmt, Args ... a)
    {
        DLogMaster (&gl_raw_context, true).with_header(fname).log(fmt, a...);
    }

    template <class ... Args>
    void log(const char* fname, DLogContext* c, const char* fmt, Args ... a) //to any context with func name
    {
        DLogMaster (c, true).with_header(fname).log(fmt, a...);
    }

    void sep(DLogContext* c = nullptr);
    //---------------------------------------------------------------------------------------
#define DLM_RAW DLogMaster(&gl_raw_context, true).with_header(__func__)
#define DLM_ERR DLogMaster(&gl_error_context, true).with_header(__func__)
#define DLM_INF DLogMaster(&gl_info_context, true).with_header(__func__)

#define DLM_CRAW(...)   log(__func__, __VA_ARGS__)
#define DLM_CERR(...)   log(__func__, &gl_error_context,__VA_ARGS__)
#define DLM_CINF(...)   log(__func__, &gl_info_context, __VA_ARGS__)
#define DLM_CLOG(...)   log(__func__, __VA_ARGS__)

#define DLM_SEP(context) sep(context);
}


/*
 test block:
    void*p = nullptr;

    gl_info_context.header_set_all(false, true, true, false, false, false);
    gl_error_context.header_set_all(false, true, true, false, false, false);

    gl_error_context.parse_console(true);
    gl_error_context.file_add("/root/logs1", "logs1");
    gl_error_context.file_add("/root/logs2", "logs2");
    gl_error_context.header_set_message("THIS IS ERROR");

    gl_error_context.format_set_flag(DLogContext::format_flag::FLUSH_CONTENT);


    DLM_RAW << "HELLO RAW STREAM" << 123 << 87.023;
    DLM_ERR << "HELLO ERROR STREAM" << false << 45.67<<p;
    DLM_INF << "HELLO INFO STREAM" << true << 23.89;

    gl_error_context.file_lock("logs2");

    DLM_CRAW("C style raw stream %d %s %f %p", 453, "TEST", 98.23, p);
    DLM_CINF("C style info stream %d %s %f %p", 453, "TEST", 98.23, p);
    DLM_CERR("C style error stream %d %s %f %p", 453, "TEST", 98.23, p);

    log("log1 test: %d %s %d %s", 123, "qwe", 456, "zxc");
    log(&gl_error_context, "log2 test: %d %s %d %s", 123, "qwe", 456, "zxc");
    log("MAIN_FUNC", "log3 test: %d %s %d %s", 123, "qwe", 456, "zxc");
    log("MAIN_FUNC", &gl_info_context, "log4 test: %d %s %d %s", 123, "qwe", 456, "zxc");


    lmraw << "RAW OBJECT:" << 456 << p << lmraw;
    lmerror() << "ERRR OBJECT:" << 456 << p << lmerror;
    lminfo << "INFO OBJECT:" << 456 << p << lminfo;
 */
#endif // DLOGS_H
