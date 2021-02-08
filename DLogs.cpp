#include "DLogs.h"
using namespace DLogs;

#define PUSH_TO_BUFFER(...) do{ \
    int n = sprintf(b_iterator,__VA_ARGS__); \
    if(n>0) b_iterator+=n; \
}while(0)


void DLogs::default_c_print(const char* line)
{
    printf("%s",line);
}
void DLogs::default_f_print(FILE* f, const char* line)
{
    fwrite(line, 1, strlen(line), f);
}

DLogContext DLogs::gl_raw_context("Raw");
DLogContext DLogs::gl_error_context("Error", "Error occur");
DLogContext DLogs::gl_info_context("Info", "Info");

DLogMaster DLogs::lmraw(&gl_raw_context);
DLogMaster DLogs::lmerror(&gl_error_context);
DLogMaster DLogs::lminfo(&gl_info_context);
//--------------------------------------------------------------------------------------- IFACE:
DLogContext::DLogContext(const char* sn, const char* lm)
{
    start_init();
    if(sn) stream_name = copy_line(sn);
    if(lm) header_set_message(lm);
}
DLogContext::~DLogContext()
{
    pfd fd = first_file;
    while(fd)
    {
        parse_file_info(fd->file, 'r');
        pfd n = fd->next;
        free_fd(fd);
        fd = n;
    }

    free_line(buffer);
    free_line(stream_name);
    free_line(header_message);
}
//---------------------------------------------------------------------------------------
void DLogContext::file_add   (const char* path, const char *key)
{
    file_remove(key);
    parse_file(true);

    FILE* f = nullptr;
    if( (f = fopen(path, "w")) )
    {
        pfd file_desc = alloc_fd();
        file_desc->file = f;
        file_desc->path = copy_line(path);
        file_desc->key  = copy_line(key);

        if(first_file)
        {
            file_desc->next = first_file;
            first_file = file_desc;
        }
        else
            first_file = file_desc;

        parse_file_info(f, 'a');
    }
}
void DLogContext::file_lock  (const char* key)
{
    pfd currf = first_file;
    while(currf)
    {
        if( strcmp(currf->key, key) == 0 )
        {
            parse_file_info(currf->file, 'l');
            currf->is_locked = true;
        }
        currf = currf->next;
    }
}
void DLogContext::file_unlock(const char* key)
{
    pfd currf = first_file;
    while(currf)
    {
        if( strcmp(currf->key, key) == 0 )
        {
            currf->is_locked = false;
            parse_file_info(currf->file, 'u');
        }
        currf = currf->next;
    }
}
void DLogContext::file_remove(const char* key)
{
    pfd prevf = nullptr;
    pfd currf = first_file;
    while(currf)
    {
        if( strcmp(currf->key, key) == 0 )
        {
            parse_file_info(currf->file, 'r');
            if(prevf) prevf->next = currf->next;
            else first_file = currf->next;

            free_fd(currf);
            return;
        }
        prevf = currf;
        currf = currf->next;
    }
}
//---------------------------------------------------------------------------------------
void DLogContext::header_set_all(const bool &date, const bool &fname, const bool &ltime, const bool &rtime, const bool &mesnum, const bool &sname)
{
    hr.cdate = date;
    hr.fname = fname;
    hr.ltime = ltime;
    hr.rtime = rtime;
    hr.mnumb = mesnum;
    hr.sname = sname;
}
void DLogContext::header_set_message(const char *ln)
{
    if(ln)
    {
        int l = strlen(ln);
        if(l)
        {
            l = (l>64)? 64:l;
            free_line(header_message);
            header_message = nullptr;
            header_message = alloc_line(l + 6);
            int n = snprintf(header_message, 64, "< %s", ln);
            sprintf(header_message+n, " > ");
        }
    }
}
void DLogContext::separator_set_size(const int& size)
{
    if(size)
    {
        free_line(separator);
        sep_size = size;
        separator = alloc_line(sep_size+0); //2
//        separator[0] = '\n';
//        separator[size+1] = '\n';
        for(int i=0;i!=size;++i) separator[i+0] = sep_symb; //1
    }
}
void DLogContext::separator_set_symb(const char& symb)
{
    sep_symb = symb;
    for(int i=0;i!=sep_size;++i) separator[i+0] = sep_symb;//1
}
void DLogContext::buffer_set_size(const int& size)
{
    if(size > 0 && size < DLOGM_BUFFER_MAX_SIZE)
    {
        free_line(buffer);
        buffer_size = size;
        buffer = alloc_line(size);
    }
}
//--------------------------------------------------------------------------------------- INNER:
void DLogContext::start_init()
{
    buffer_size = DLOGM_BUFFER_SIZE;
    buffer   = alloc_line(DLOGM_BUFFER_SIZE);
    b_iterator = buffer;


    separator = nullptr;
    sep_symb = DLOGM_SEP_SYMB;
    separator_set_size(DLOGM_SEP_SIZE);


    first_file     = nullptr;
    stream_name    = nullptr;
    header_message = nullptr;

    parse_to_file    = false;
    parse_to_console = true;

    hr.cdate = false;
    hr.fname = false;
    hr.ltime = false;
    hr.mnumb = false;
    hr.rtime = false;
    hr.sname = false;

    fflags   = 0;
    float_precision  = 4;
    double_precision = 4;

    c_print = default_c_print;
    f_print = default_f_print;
}
void DLogContext::out(const char* o)
{
    if(parse_to_console)
    {
        c_print(o);
    }
    if(parse_to_file)
    {
        pfd curr = first_file;
        while(curr)
        {
            if(!curr->is_locked) f_print(curr->file, o);
            curr = curr->next;
        }
    }
}
void DLogContext::flush(const char* line)
{
    if(line)
        out(line);
    else
    {
        out(buffer);
        zero_line(buffer, buffer_size);
        b_iterator = buffer;
    }
    if(fflags & FLUSH_ALL) new_line();
}

void DLogContext::clear_buffer()
{
    buffer[0] = '\0';
    b_iterator = buffer;
}
void DLogContext::new_line()
{
    out("\n");
}
void DLogContext::add_header(const char *fname)
{
    time_t t = time(NULL);
    struct tm now = *localtime(&t);

    if(hr.cdate)
    {
        PUSH_TO_BUFFER("[D: %d.%d.%d]", now.tm_mday, now.tm_mon+1, now.tm_year+1900);
    }
    if(hr.mnumb)
    {
        PUSH_TO_BUFFER("[N: %d]", 0);
    }
    if(hr.ltime)
    {
        PUSH_TO_BUFFER("[L: %d:%d:%d]", now.tm_hour, now.tm_min, now.tm_sec);
    }
    if(hr.rtime)
    {
        int ms = clock();
        int s = ms / 1000;  ms %= 1000;
        int m = s  / 60;    s  %= 60;
        int h = m  / 60;    m  %= 60;

        PUSH_TO_BUFFER("[R: %d:%d:%d:%d]", h, m, s, ms);
    }
    if(hr.fname && fname)
    {
        PUSH_TO_BUFFER("[F: %s]", fname);
    }
    if(hr.sname && stream_name)
    {
        PUSH_TO_BUFFER("[S: %s]", stream_name);
    }
    if(b_iterator != buffer)
        sprintf(b_iterator, " ");
    flush();

    if(  check_fflag(FLUSH_MESSAGE) && !check_fflag(FLUSH_ALL)  ) new_line();
    if(header_message)
    {
        flush(header_message);
        if(  check_fflag(FLUSH_CONTENT) && !check_fflag(FLUSH_ALL)  ) new_line();
    }
}
bool DLogContext::check_fflag(DLogContext::format_flag f)
{
    return (bool)(fflags&f);
}
//---------------------------------------------------------------------------------------
DLogContext::pfd DLogContext::alloc_fd()
{
    pfd __fd = (pfd)malloc(sizeof(file_descriptor));
    if(__fd)
    {
        memset(__fd, 0, sizeof(file_descriptor));
        __fd->file = nullptr;
        __fd->next = nullptr;
        __fd->path = nullptr;
        __fd->key = nullptr;
        __fd->is_locked = false;
    }
    return __fd;
}
void DLogContext::free_fd(DLogContext::pfd __fd)
{
    if(__fd)
    {
        if(__fd->file) {fclose(__fd->file);    __fd->file = nullptr;}
        if(__fd->path) {free_line(__fd->path); __fd->path = nullptr;}
        if(__fd->key)  {free_line(__fd->key);  __fd->key  = nullptr;}
        __fd->next = nullptr;
        __fd->is_locked = false;
        free(__fd);
    }
}

void DLogContext::parse_file_info(FILE *f, const char &state)
{
    switch(state)
    {
    case 'a':
        PUSH_TO_BUFFER("Start loging to this file");
        break;
    case 'r':
        PUSH_TO_BUFFER("Stop loging to this file");
        break;
    case 'l':
        PUSH_TO_BUFFER("This file locked for loging");
        break;
    case 'u':
        PUSH_TO_BUFFER("This file unlocked for loging");
        break;
    }

    PUSH_TO_BUFFER("< ");
    parse_date();
    parse_ltime();
    parse_rtime();
    PUSH_TO_BUFFER(" >\n");
    f_print(f, buffer);
    clear_buffer();
}
void DLogContext::parse_date(char *to)
{
    time_t t = time(NULL);
    struct tm now = *localtime(&t);

    if(to) sprintf(to, "[D: %d.%d.%d]", now.tm_mday, now.tm_mon+1, now.tm_year+1900);
    else   PUSH_TO_BUFFER("[D: %d.%d.%d]", now.tm_mday, now.tm_mon+1, now.tm_year+1900);

}
void DLogContext::parse_ltime(char *to)
{
    time_t t = time(NULL);
    struct tm now = *localtime(&t);
    if(to) sprintf(to, "[L: %d:%d:%d]", now.tm_hour, now.tm_min, now.tm_sec);
    else  PUSH_TO_BUFFER("[L: %d:%d:%d]", now.tm_hour, now.tm_min, now.tm_sec);
}
void DLogContext::parse_rtime(char *to)
{
    int ms = clock();
    int s = ms / 1000;  ms %= 1000;
    int m = s  / 60;    s  %= 60;
    int h = m  / 60;    m  %= 60;
    if(to) sprintf(to, "[R: %d:%d:%d:%d]", h, m, s, ms);
    else   PUSH_TO_BUFFER("[R: %d:%d:%d:%d]", h, m, s, ms);
}
//---------------------------------------------------------------------------------------
char *DLogContext::alloc_line(const int &size)
{
    char* nl = (char*)malloc(size+1);
    if(nl) memset(nl, 0, size+1);
    nl[size] = '\0';
    return nl;
}
char *DLogContext::copy_line(const char* from)
{
    int s = strlen(from);
    if(s>0)
    {
        char* nl = (char*)malloc(s+1);
        memcpy(nl, from, s);
        nl[s] = '\0';
        return nl;
    }
    else
        return nullptr;
}
void  DLogContext::free_line(char* line)
{
    if(line)
    {
        memset(line, 0, strlen(line));
        free(line);
    }
}
void  DLogContext::zero_line(char *line, const int &size)
{
    int s = size? size:strlen(line);
    memset(line, 0, s);
}
//===============================================================================================================
//===============================================================================================================
DLogMaster::DLogMaster(DLogContext *c)
{
    if(c) context = c;
    else
    {
        context = &gl_raw_context;
    }
}
DLogMaster::~DLogMaster()
{
    if(!context->check_fflag(DLogContext::format_flag::FLUSH_ALL)) context->new_line();
}
DLogMaster &DLogMaster::operator()(const char *_caller_name)
{
    context->add_header(_caller_name);
    return *this;
}
DLogMaster &DLogMaster::with_header(const char *_caller_name)
{
    context->add_header(_caller_name);
    return *this;
}
void DLogMaster::sep()
{
    context->flush(context->separator);
}

DLogMaster &DLogMaster::operator <<(const int &v)
{
    sprintf(context->buffer, "%d ", v);
    context->flush();
    return *this;
}
DLogMaster &DLogMaster::operator <<(const char *v)
{
    sprintf(context->buffer, "%s ", v);
    context->flush();
    return *this;
}
DLogMaster &DLogMaster::operator <<(const float &v)
{
    sprintf(context->buffer, "%.*f ", context->float_precision, v);
    context->flush();
    return *this;
}
DLogMaster &DLogMaster::operator <<(const double &v)
{
    sprintf(context->buffer, "%.*f ", context->double_precision, v);
    context->flush();
    return *this;
}
DLogMaster &DLogMaster::operator <<(const bool &v)
{
    if(v) context->flush("true ");
    else  context->flush("false ");

    return *this;
}
DLogMaster &DLogMaster::operator <<(const long &v)
{
    sprintf(context->buffer, "%ld ", v);
    context->flush();
    return *this;
}
DLogMaster &DLogMaster::operator <<(void *p)
{
    sprintf(context->buffer, "%p ", p);
    context->flush();
    return *this;
}
DLogMaster &DLogMaster::operator <<(DLogMaster& lm)
{
    if(&lm == this)
    {
        sprintf(context->buffer, "\n");
        context->flush();
    }
    return *this;
}
DLogMaster &DLogMaster::operator <<(separator_t)
{
    if(!context->check_fflag(DLogContext::format_flag::FLUSH_ALL)) context->new_line();
    sep();
    if(!context->check_fflag(DLogContext::format_flag::FLUSH_ALL)) context->new_line();
    return *this;
}
//---------------------------------------------------------------------
void DLogs::sep(DLogContext *c)
{
    DLogMaster(c).sep();
}
