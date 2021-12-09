#include "DLexeme.h"
#include "daran.h"

void DLexeme::_init_deafault_context()
{
    default_context = get_mem<read_context>(1);
    default_context->min = 0;
    default_context->max = 0;
    default_context->use_min = false;
    default_context->use_max = false;
    default_context->unique = false;
    default_context->max_size = 0;
}
DLexeme::DLexeme()
{
    _init_deafault_context();
    inner_rule = NULL;
}
DLexeme::DLexeme(DLexeme::raw rule)
{
    _init_deafault_context();
    inner_rule = NULL;
    set_rule(rule);
}
DLexeme::~DLexeme()
{
    free_mem(default_context);
}
const DLexeme::lexeme DLexeme::share_null = {nullptr, nullptr, 0};
inline bool DLexeme::is_digit(char c)
{return (c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6' || c == '7' || c == '8' || c == '9' || c == '0');}
bool DLexeme::is_number(const DLexeme::lexeme &l)
{
    if(l.size > 10) return false;
    raw b = l.begin;
    raw e = l.begin + l.size;
    while(b!=e) if(!is_digit(*b++)) return false;
    return true;
}
bool DLexeme::is_special(char c, DLexeme::raw s)
{
    while(*s!='\0') if(c == *s++) return true;
    return false;
}
bool DLexeme::base_equal(const lexeme &l, DLexeme::raw sample)
{
    return strlen(sample) == (size_t)l.size && buffer_compare(l.begin, sample, l.size);
}

DLexeme::lexeme DLexeme::base(DLexeme::raw v, DLexeme::raw end, DLexeme::raw special)
{
    lexeme l = share_null;
    if(v == nullptr) return l;
    if(!special) special = " ";
    void *start = (void*)v;
//    qDebug()<<"base::: start:"<<start<<"end:"<<(void*)end;
    for(;v != end;++v)
    {
        if(end == nullptr && *v == '\0')
        {
//            qDebug()<<"end in nullptr and sym is term zero";
            break;
        }

//        qDebug()<<"base: sym:"<<*v<<"start:"<<start;
        if(is_special(*v, special))
        {
            if(l.begin)
            {
//                qDebug()<<"----sym:"<<*v<<"is special";
                break;
            }
        }
        else
        {
            if(!l.begin) l.begin = v;
            ++l.size;
        }
//        if(*(v+1) == '\0') qDebug()<<"next sym is term 0";
//        if((v+1) == end) qDebug()<<"next sym is END";
    }
    l.end = l.begin + l.size;

//    qDebug()<<"return lexeme:"<<(void*)l.begin;
    return l;
}
size_t DLexeme::sym_base_len(DLexeme::raw v, DLexeme::raw end, char c)
{
    int i=0;
    while(*v != '\0' && v != end) {if(*v == c) return i; ++v; ++i;}
    return 0;
}
DLexeme::lexeme DLexeme::sym_base(DLexeme::raw v, DLexeme::raw end, char c)
{
    while(*v != '\0' && v != end) {if(*v == c) return {v, v+1, 1}; ++v;}
    return share_null;
}
DLexeme::lexeme DLexeme::strict_sym_base(DLexeme::raw v, DLexeme::raw end, char c, DLexeme::raw special)
{
    if(!special) special = " ";
    while(*v != '\0' && v != end)
    {
        if(*v == c) return {v, v+1, 1};
        else if(!is_special(*v, special)) return share_null;
        ++v;
    }
    return share_null;
}
DLexeme::lexeme DLexeme::sym_unspecial(DLexeme::raw v, DLexeme::raw end, DLexeme::raw special)
{
    if(!special) special = " ";
    while(*v != '\0' && v != end)
    {
        if(!is_special(*v, special)) return {v, v+1, 1};
        ++v;
    }
    return share_null;
}

DLexeme::lexeme DLexeme::sym_stop_on(DLexeme::raw v, DLexeme::raw end, char c)
{
    while(*v != '\0' && v != end)
    {
        if(*v == c) return {v, v+1, 1};
        ++v;
    }
    return {v, v+1, 1};
}
DArray<int> DLexeme::numbers(DLexeme::raw v, DLexeme::raw end, DLexeme::read_context *context, DLexeme::raw special)
{
    DArray<int> list;
    lexeme l;
    read_context* c = context? context:default_context;
    if(!special) special = " ,";
    l = base(v, end, special);
    while(l.begin)
    {
        value_t number = 0;
        if(is_number(l))
        {
            number = atoi(l.begin);
            if(
                  (!c->use_min       || number >= c->min)
               && (!c->use_max       || number <= c->max)
               && (!c->unique        || !list.count(number))
               && (!c->max_size      || (index_t)list.size() < c->max_size )
               ) list.push_back(number);
        }
        l = base(l.end,end,special);
    }
    return list;
}
DArray<int> DLexeme::list(DLexeme::raw v, DLexeme::raw end, DLexeme::raw special, char sep, DLexeme::read_context *context)
{
    DArray<int> list;
    lexeme l;
    read_context* c = context? context:default_context;
    if(!special) special = " ,";
    l = base(v, end, special);
    while(l.begin)
    {
        if(is_number(l))
        {
            value_t number = atoi(l.begin);
            if(
                  (!c->use_min      || number >= c->min)
               && (!c->use_max      || number <= c->max)
               && (!c->unique       || !list.count(number))
               && (!c->max_size     || (index_t)list.size() < c->max_size )
               ) list.push_back(number);
            l = strict_sym_base(l.end, end, sep, special);
        }
        else
        {
            if(l.size > 1 && !list.empty()) break;
            l = base(l.end,end, special);
        }
    }
    return list;
}
DArray<char> DLexeme::options(DLexeme::raw v, DLexeme::raw end, DLexeme::raw special, char prefix, DLexeme::read_context *context)
{
    DArray<char> list;
    lexeme l;
    read_context* c = context? context:default_context;
    if(!special) special = " ,";
    l = base(v, end, special);
    while(l.begin)
    {
        if(l.size == 2 && *l.begin == prefix)
        {
            char o = *(l.begin + 1);
            if(
                  (!c->use_min      || o >= (char)c->min)
               && (!c->use_max      || o <= (char)c->max)
               && (!c->unique       || !list.count(o))
               && (!c->max_size     || (index_t)list.size() < c->max_size )
               ) list.push_back(*(l.begin + 1));
        }
        l = base(l.end,end, special);
    }
    return list;
}
int DLexeme::number(DLexeme::raw v, DLexeme::raw end, DLexeme::raw special, DLexeme::read_context *context)
{
    value_t number;
    lexeme l;
    read_context* c = context? context:default_context;
    if(!special) special = " ,";
    l = base(v, end, special);
    while(l.begin)
    {
        v = l.end;
        if(is_number(l))
        {
            number = atoi(l.begin);
            if(
                  (!c->use_min  || number >= c->min)
               && (!c->use_max  || number <= c->max)
               ) return number;
        }
     l = base(l.end, end, special);
    }
    return 0;
}
char DLexeme::option(DLexeme::raw v, DLexeme::raw end, DLexeme::raw special, char prefix, DLexeme::read_context *context)
{
    char o;
    lexeme l;
    read_context* c = context? context:default_context;
    if(!special) special = " ,";
    l = base(v, end, special);
    while(l.begin)
    {
        if(l.size == 2 && *l.begin == prefix)
        {
            o = *(l.begin + 1);
            if(
                  (!c->use_min || o >= c->min)
               && (!c->use_max || o <= c->max)
               ) return o;
        }
     l = base(l.end, end, special);
    }
    return 0;
}
DLexeme::lexeme DLexeme::find(DLexeme::raw v, DLexeme::raw end, DLexeme::raw rule, DLexeme::raw special)
{
    if(v == NULL) return share_null;
    raw __rule = rule ? rule : inner_rule;
    if(__rule == NULL) return share_null;

    lex_context c;
    int r = read_rule(__rule, c);
    if(r < 0)
    {
        return share_null;
    }
    lexeme l;
    DLexeme::raw __special = special ? special : c.inner_special;

//    qDebug()<<"find: __special:"<<__special;
    value_t cnt = 0;
    while ( (l = base(v, end, __special)).begin )
    {
//        qDebug()<<"check lexeme:"<<lexeme_to_string(l).c_str();
        if(_check_lex(&l, &c))
        {
//            qDebug()<<"---- check: true";
            if(++cnt > c.skip) return l;
        }
        v = l.end;
    }
    return share_null;
}

void DLexeme::set_rule(DLexeme::raw rule)
{
    if(rule)
    {
        int s = strlen(rule);
        if(s > 0)
        {
            if(  (inner_rule = reget_zmem(inner_rule, s)) != NULL)
                copy_mem(inner_rule, rule, s);
        }
    }
}
bool DLexeme::check_rule(DLexeme::raw rule)
{
    raw __rule = rule ? rule : inner_rule;
    if(__rule == NULL) return false;
    lexeme l;
    auto check = __rule;
    while(  (l = sym_base(check, nullptr, '<')).begin  )
    {
        if( !(l = sym_base(l.end, nullptr, '>')).begin)
        {
            return false;
        }
        check = l.end;
    }
    return true;
}


void DLexeme::shift_lex(DLexeme::lexeme *l, int s)
{
    if(l->begin && s <= l->size)
    {
        l->begin += s;
        l->size  -= s;
    }
    else *l = share_null;
}
int DLexeme::read_rule(DLexeme::raw rule, lex_context &c)
{
    c.min_len = 0;
    c.max_len = 0;
    c.skip = 0;
    c.direct_mode = true;
    c.inner_special = NULL;

    lexeme l;
    raw special1 = " =|:>#,";
    raw special2 = " :>";
    auto check = rule;

    while(  (l = sym_base(check, nullptr, '<')).begin  )
    {
        if( !(l = sym_base(l.end, nullptr, '>')).begin)
        {
            printf("read_rule: Syntax error with <> blocks\n");
            return -1;
        }
        check = l.end;
    }
    while((l = sym_base(rule, nullptr, '<')).begin)
    {
        raw block_it = l.end;
        size_t block_size = sym_base_len(block_it, nullptr, '>');
        raw block_end = block_it + block_size;

        if( (l = strict_sym_base(block_it, block_end, '$')).begin)
        {
            block_it = l.end;
            if( (l = base(block_it, block_end, special2)).begin)
            {
                lexeme local;
                value_t* val_p = nullptr;
                value_t value = 0;
                block_it = l.end;

                int undefined_block = 1;

                if(undefined_block && base_equal(l, "min"))
                {
                    val_p = &c.min_len;
                    undefined_block = 0;
                }
                if(undefined_block && base_equal(l, "max"))
                {
                    val_p = &c.max_len;
                    undefined_block = 0;
                }
                if(undefined_block && base_equal(l, "len")
                   && (local = sym_base(block_it, block_end, ':')).begin
                        )
                {
                    c.len_wl = list(local.end, block_end, special1);
                    undefined_block = 0;
                }
                if(undefined_block && base_equal(l, "nodirect"))
                {
                    c.direct_mode = false;
                    undefined_block = 0;
                }
                if(undefined_block && base_equal(l, "spec")
                        && (local = sym_base(block_it, block_end, ':')).begin)
                {
                    auto __special_lexeme = base(local.end, block_end, ">");
                    c.inner_special = get_zmem<char>(__special_lexeme.size);
                    copy_mem(c.inner_special, __special_lexeme.begin, __special_lexeme.size);

                    undefined_block = 0;
                }
                if(undefined_block && base_equal(l, "skip"))
                {
                    val_p = &c.skip;
                    undefined_block = 0;
                }


                if(val_p
                   && ( block_it = sym_base(block_it, block_end, ':').begin )
                   && ( l = base(block_it, block_end, special1)).begin
                   && is_number(l)
                   && (value = atoi(l.begin)) > 0
                        ) *val_p = value;
            }
            else
            {
            }
        }
        else
        {
            sublex* head_sl = nullptr;
            do
            {
                if((l = base(block_it, block_end, special1)).begin)
                {
                    sublex* sl = get_mem<sublex>(1);
                    sl->next_or = nullptr;
                    sl->n = 1;
                    sl->type = 0;
                    sl->l = l;

                    if(*(l.begin-1) == '#') sl->type = 2;
                    else if(*(l.begin-1) == '=') sl->type = 3;
                    else if(*l.end == '#') sl->type = 1;

                    if(sl->type == 0 && strict_sym_base(l.end, block_end, ':').begin)
                    {
                        l = base(l.end, block_end, special1);
                        if(is_number(l)) sl->n = atoi(l.begin);
                    }

                    if(head_sl) sl->next_or = head_sl;
                    head_sl = sl;
                    block_it = l.end;
                }
            } while(  (block_it = sym_base(block_it, block_end, '|').begin) );
            if(head_sl) c.sub_lex.push_back(head_sl);
        }
        rule = block_end;
    }
    return 0;
}
bool DLexeme::__check_block(DLexeme::sublex *sl, DLexeme::lexeme *l, bool direct_mode)
{
    int s=0;
    while(sl)
    {
        if(sl->l.size <= l->size)
            switch (sl->type)
            {
            case 0:
            {
                bool res = true;
                for(int i=0;i!=sl->n;++i)
                {
                    if( (s = find_bytes_pos(sl->l.begin, l->begin + s, sl->l.size, l->size - s) ) < 0)
                    {
                        res = false;
                        break;
                    }
                    else ++s;
                }
                if(res) return res;
                if(direct_mode && s>=0) shift_lex(l, s+sl->l.size);
                break;

            }

            case 1: if( buffer_compare(sl->l.begin, l->begin, sl->l.size)) {if(direct_mode) shift_lex(l, sl->l.size);return true;} break;
            case 2: if( buffer_compare(sl->l.begin, l->begin + (l->size - sl->l.size), sl->l.size)) {if(direct_mode) *l = share_null; return true;} break;
            case 3: if( sl->l.size == l->size && buffer_compare(sl->l.begin, l->begin, l->size)) {if(direct_mode) *l = share_null;return true;} break;

            }
        sl = sl->next_or;
    }
    return false;
}
bool DLexeme::_check_lex(DLexeme::lexeme *l, DLexeme::lex_context *c)
{
    if(
            l->size >= c->min_len
            && (!c->max_len || l->size <= c->max_len)
      )
    {
        bool wrong_size = c->len_wl.empty() ? false : true;
        for(int i=0;i!=c->len_wl.size();++i) if(l->size == c->len_wl[i]) wrong_size = false;
        if(wrong_size) return false;

        auto b = c->sub_lex.begin();
        auto e = c->sub_lex.end();
        lexeme cp_l = *l;
        while( b != e)
        {
            if(l->size == 0) return false;
            if(!__check_block(*b, &cp_l, c->direct_mode)) return false;
            ++b;
        }
    }
    else return false;
    return true;
}


void lexeme_print(const DLexeme::lexeme &l, bool show_size)
{
    std::cout << '[';
    auto b = l.begin;
    while( b != l.end ) std::cout << *b++;
    std::cout <<"]";
    if(show_size) std::cout << " (" << l.size << ')';
    std::cout << std::endl;
}
void lexeme_parse(const DLexeme::lexeme &l, char *buffer, size_t buffer_size)
{
    int n = buffer_size < (size_t)l.size ? buffer_size : l.size;
    snprintf(buffer, n, l.begin);
}
std::string lexeme_to_string(const DLexeme::lexeme &l)
{
    if(l.begin && l.size > 0)
        return std::string(l.begin, l.size);
    return  std::string();
}
int lexeme_to_number(const DLexeme::lexeme &l)
{
    if(l.begin && l.size > 0)
        return DLexeme().number(l.begin, l.end);
    return 0;
}
int lexeme_force_number(const DLexeme::lexeme &l, int big_endian)
{
    int v = 0;
    if(big_endian)
        D_WB32_P2P(&v, l.begin);
    else
        D_WL32_P2P(&v, l.begin);

    return v;
}
