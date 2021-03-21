#ifndef DLEXEME_H
#define DLEXEME_H

#include <stdint.h>
#include "daran.h"
#include "DArray.h"
class DLexeme
{
public:
    typedef const char* raw;
    typedef uint64_t pos_t;
    typedef uint32_t index_t;
    typedef int64_t value_t;

    DLexeme();
    ~DLexeme();


    struct lexeme
    {
        raw begin;
        raw end;
        int size;
    };
    struct sublex
    {
        int type; //0 - sublex, 1 - beg, 2 - end, 3 - equal
        int n;
        lexeme l;
        sublex* next_or;
    };
    struct read_context
    {
        int min; bool use_min;
        int max; bool use_max;
        bool unique;
        index_t max_size;
        void set_min(int v){min = v; use_min = true;}
        void disable_min(){use_min = false;}
        void set_max(int v){max = v; use_max = true;}
        void desable_max(){use_max = false;}
    };
    struct lex_context
    {
        value_t min_len;
        value_t max_len;
        DArray<int> len_wl; //white list
        DArray<sublex*> sub_lex;
        bool direct_mode;
    };

    static const lexeme share_null;

    inline bool is_digit(char c);
    inline bool is_number(const lexeme& l);
    inline bool is_special(char c, raw s);
    bool base_equal(const lexeme& l, raw sample);

    void print_lexeme(const lexeme& l, bool show_size = true);

    lexeme base(raw v, raw end = nullptr, raw special = nullptr);
    size_t sym_base_len(raw v, raw end, char c);
    lexeme sym_base(raw v, raw end, char c);
    lexeme strict_sym_base(raw v, raw end, char c, raw special = nullptr);

    DArray<int> numbers(raw v, raw end = nullptr, read_context* context = nullptr, raw special = nullptr);
    DArray<int> list(raw v, raw end = nullptr, raw special = nullptr, char sep = ',', read_context* context = nullptr );
    DArray<char> options(raw v, raw end, raw special = nullptr, char prefix = '-', read_context* context = nullptr);
    int number(raw v, raw end, raw special = nullptr, read_context* context = nullptr);
    char option(raw v, raw end, raw special = nullptr, char prefix = '-', read_context* context = nullptr);
    lexeme lex(raw v, raw end, raw rule, raw special = nullptr);

    read_context* default_context;
private:
    void print_lex_context(const lex_context& c);
    void shift_lex(lexeme* l, int s);
    lex_context read_rule(raw rule);
    inline bool __check_block(sublex* sl, lexeme* l, bool direct_mode = false);
    inline bool _check_lex(lexeme* l, lex_context* c);
};

#endif // DLEXEME_H
