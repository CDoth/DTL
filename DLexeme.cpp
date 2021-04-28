#include "DLexeme.h"

#include <QDebug>
DLexeme::DLexeme()
{
    default_context = get_mem<read_context>(1);
    default_context->min = 0;
    default_context->max = 0;
    default_context->use_min = false;
    default_context->use_max = false;
    default_context->unique = false;
    default_context->max_size = 0;
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
void DLexeme::print_lexeme(const lexeme &l, bool show_size)
{
    auto b = l.begin;
    while( b != l.end ) std::cout << *b++;
    if(show_size) std::cout << '(' << l.size << ')';
    std::cout << std::endl;
}
DLexeme::lexeme DLexeme::base(DLexeme::raw v, DLexeme::raw end, DLexeme::raw special)
{
    lexeme l = share_null;
    if(v == nullptr) return l;
    if(!special) special = " ";
    for(;*v!='\0' && v != end;++v)
    {
        if(is_special(*v, special))
        {
            if(l.begin)break;
        }
        else
        {
            if(!l.begin) l.begin = v;
            ++l.size;
        }
    }
    l.end = l.begin + l.size;
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
DLexeme::lexeme DLexeme::lex(DLexeme::raw v, DLexeme::raw end, DLexeme::raw rule, DLexeme::raw special)
{
    lex_context c = read_rule(rule);
    lexeme l;
    while ( (l = base(v, end, special)).begin )
    {
        if(_check_lex(&l, &c)) return l;
        v = l.end;
    }
    return share_null;
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
DLexeme::lex_context DLexeme::read_rule(DLexeme::raw rule)
{
    lex_context context;
    context.min_len = 0;
    context.max_len = 0;
    context.direct_mode = true;
    lexeme l;
    raw special1 = " =|:>#,";
    raw special2 = " :>";
    auto check = rule;

    while(  (l = sym_base(check, nullptr, '<')).begin  )
    {
        if( !(l = sym_base(l.end, nullptr, '>')).begin)
        {
            printf("read_rule: Syntax error with <> blocks\n");
            return context;
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

                if(base_equal(l, "min"))
                {
                    val_p = &context.min_len;
                }
                if(base_equal(l, "max"))
                {
                    val_p = &context.max_len;
                }
                if(base_equal(l, "len")
                   && (local = sym_base(block_it, block_end, ':')).begin
                        )
                {
                    context.len_wl = list(local.end, block_end, special1);
                }
                if(base_equal(l, "nodirect"))
                {
                    context.direct_mode = false;
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
            if(head_sl) context.sub_lex.push_back(head_sl);
        }
        rule = block_end;
    }
    return context;
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
