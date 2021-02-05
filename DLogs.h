#ifndef DLOGS_H
#define DLOGS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

namespace DLogs
{
//------------------------------------------------------------------- log context:
#define DLOGM_BUFFER_SIZE 1024
#define DLOGM_BUFFER_MAX_SIZE 1024 * 12
#define DLOGM_FUNC_NAME __func__
#define DLOGM_SEP_SIZE 15
#define DLOGM_SEP_SYMB '-'


    class DLogMaster;
    class DLogContext
    {
    public:
        friend class DLogMaster;
        DLogContext(const char* stream_name = nullptr, const char* log_message = nullptr);
        ~DLogContext();

        //----------parse
        void parse_console(const bool& _state) {parse_to_console = _state;}
        void parse_file   (const bool& _state) {parse_to_file    = _state;}

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


        void header_set_all(const bool& date   = false,
                            const bool& fname  = false,
                            const bool& ltime  = false,
                            const bool& rtime  = false,
                            const bool& mesnum = false,
                            const bool& sname  = false);
        void header_set    (const header_resolution& _hr) {hr = _hr;}

        void header_date_visibility  (const bool& _state) {hr.cdate = _state;}
        void header_fname_visibility (const bool& _state) {hr.fname = _state;}
        void header_ltime_visibility (const bool& _state) {hr.ltime = _state;}
        void header_rtime_visibility (const bool& _state) {hr.mnumb = _state;}
        void header_mesnum_visibility(const bool& _state) {hr.rtime = _state;}
        void header_sname_visibility (const bool& _state) {hr.sname = _state;}

        void header_set_message(const char* ln);
        //----------separator
        void separator_set_size(const int&  size);
        void separator_set_symb(const char& symb);
        //----------precision
        void precision_float (const int& p) {float_precision  = p;}
        void precision_double(const int& p) {double_precision = p;}
        //----------buffer
        void buffer_set_size(const int& size);

        enum format_flag
        {
            FLUSH_ALL     = 0b00000001,
            FLUSH_MESSAGE = 0b00000010,
            FLUSH_CONTENT     = 0b00000100,
        };
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
        char* alloc_line(const int& size);
        char* copy_line(const char* from);
        void  free_line(char* line);
        void  zero_line(char* line, const int& size = 0);
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
        DLogMaster(DLogContext* c = nullptr);
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
        DLogMaster &operator <<(DLogMaster&);
        DLogMaster &operator <<(separator_t);


        template <class ... Args>
        void log(const char* fmt, Args ... a)
        {
            snprintf(context->buffer, DLOGM_BUFFER_SIZE ,fmt, a...);
            context->flush();
        }
    private:
        DLogContext* context;
    };

    extern DLogMaster lmraw;
    extern DLogMaster lmerror;
    extern DLogMaster lminfo;

    template <class ... Args>
    void log(const char* fmt, Args ... a) //to raw context
    {
        DLogMaster().with_header().log(fmt, a...);
    }

    template <class ... Args>
    void log(DLogContext* c, const char* fmt, Args ... a)
    {
        DLogMaster (c).with_header().log(fmt, a...);
    }

    template <class ... Args>
    void log(const char* fname, const char* fmt, Args ... a)
    {
        DLogMaster (&gl_raw_context).with_header(fname).log(fmt, a...);
    }

    template <class ... Args>
    void log(const char* fname, DLogContext* c, const char* fmt, Args ... a) //to any context with func name
    {
        DLogMaster (c).with_header(fname).log(fmt, a...);
    }

    void sep(DLogContext* c = nullptr);
    //---------------------------------------------------------------------------------------
#define DLM_RAW DLogMaster(&gl_raw_context).with_header(__func__)
#define DLM_ERR DLogMaster(&gl_error_context).with_header(__func__)
#define DLM_INF DLogMaster(&gl_info_context).with_header(__func__)

#define DLM_CRAW(...)   log(__func__, __VA_ARGS__)
#define DLM_CERR(...)   log(__func__, &gl_error_context,__VA_ARGS__)
#define DLM_CINF(...)   log(__func__, &gl_info_context, __VA_ARGS__)
#define DLM_CLOG(...)   log(__func__, __VA_ARGS__)

#define DLM_SEP(context) sep(context);
}
#endif // DLOGS_H
