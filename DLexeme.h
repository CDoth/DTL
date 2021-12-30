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
    DLexeme(raw rule);
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
        char *inner_special;
        DArray<char> permittedSymbols;
        bool onlyDigits;
        value_t skip;
    };

    static const lexeme share_null;

    inline bool is_digit(char c);
    inline bool is_number(const lexeme& l);
    inline bool is_special(char c, raw s);
    bool base_equal(const lexeme& l, raw sample);
    bool is_permitted_line(const lexeme &l, raw permitted, bool permitDigits);



    lexeme base(raw v, raw end, raw special = nullptr);
    lexeme base_in_syms(raw v, raw end, char left, char right, raw special = nullptr);

    size_t sym_base_len(raw v, raw end, char c);
    lexeme sym_base(raw v, raw end, char c);
    lexeme strict_sym_base(raw v, raw end, char c, raw special = nullptr);
    lexeme sym_unspecial(raw v, raw end, raw special);
    lexeme sym_stop_on(raw v, raw end, char c);

    DArray<int> numbers(raw v, raw end = nullptr, read_context* context = nullptr, raw special = nullptr);
    DArray<int> list(raw v, raw end = nullptr, raw special = nullptr, char sep = ',', read_context* context = nullptr );
    DArray<char> options(raw v, raw end = nullptr, raw special = nullptr, char prefix = '-', read_context* context = nullptr);
    int number(raw v, raw end, raw special = nullptr, read_context* context = nullptr);
    lexeme numberLexeme(raw v, raw end, raw special = nullptr, read_context *context = nullptr);
    char option(raw v, raw end, raw special = nullptr, char prefix = '-', read_context* context = nullptr);
    lexeme find(raw v, raw end, raw rule, raw special = nullptr);
    void set_rule(raw rule);
    bool check_rule(raw rule = NULL);

    read_context* default_context;
    char *inner_rule;
private:
    void _init_deafault_context();
    void shift_lex(lexeme* l, int s);
    int read_rule(raw rule, lex_context &c);
    inline bool __check_block(sublex* sl, lexeme* l, bool direct_mode = false);
    inline bool _check_lex(lexeme* l, lex_context* c);
};
void lexeme_print(const DLexeme::lexeme &l, bool show_size = true);
void lexeme_parse(const DLexeme::lexeme &l, char *buffer, size_t buffer_size);
std::string lexeme_to_string(const DLexeme::lexeme &l);
int lexeme_to_number(const DLexeme::lexeme &l);
int lexeme_force_number(const DLexeme::lexeme &l, int big_endian);
class StringCompare
{
public:
    StringCompare(const std::string &s) : __s(s) {}
    bool operator()(const std::string &s) const {return s == __s;}
private:
    std::string __s;
};



/*
 test block
    DLexeme lex;

    const char* test1 = "qwe 12a656hasd zxcsd";
    const char* test2 = "123 45 67 98 3 d 4rt ty65 23 we43xc 1 0 982 !2 ! 3";

    auto l = lex.lex(test1, nullptr, "<#sd> <$max:5>");
    DArray<int> list;
    list = lex.numbers(test2);
    for(int i=0;i!=list.size();++i) printf("%d num: %d\n",i, list[i]);

    lex.print_lexeme(l);
 */
#endif // DLEXEME_H
