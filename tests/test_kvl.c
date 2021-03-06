/* -*- coding: utf-8 -*-
###############################################################################
# Author: Alain Cady <alain.cady@bull.net>
# Created on: Nov 17, 2014
# Contributors:
#              
###############################################################################
# Copyright (C) 2014  Bull S. A. S.  -  All rights reserved
# Bull, Rue Jean Jaures, B.P.68, 78340, Les Clayes-sous-Bois
# This is not Free or Open Source software.
# Please contact Bull S. A. S. for details about its license.
###############################################################################
*/
#include <stdio.h>
#include <string.h>
#include <limits.h>

/* Cunit */
#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

/* BXI Api */
#include <bxi/base/str.h>
#include <bxi/base/log.h>
#include <bxi/util/vector.h>
#include <bxi/util/misc.h>

/* KeVaLa */
union YYSTYPE
{
    #include <bxi/util/kvl_types.h>
};
#include <bxi/util/kvl.h>

#ifndef VTEST
    #undef DEBUG
    #define DEBUG(...)
    #define log_level BXILOG_ALERT
#else
    #define log_level BXILOG_LOWEST
#endif



/* Yacc utils */
YYSTYPE _yylval;
YYLTYPE _yylloc;
FILE *fakefile;
yyscan_t _scanner;

/* Helper */
#define bxivector_compare(a, b, type) do {                                                  \
    size_t min_size = BXIMISC_MIN(bxivector_get_size(a), bxivector_get_size(b));            \
        for (size_t i=0; i<min_size; ++i) {                                                 \
            CU_ASSERT(*(type*)bxivector_get_elem(a, i) == *(type*)bxivector_get_elem(b, i));\
        }                                                                                   \
        CU_ASSERT(bxivector_get_size(a) == bxivector_get_size(b));                          \
    } while(0)

/******************************** Lexer ***********************************************/
/* Lexer test suite initialization function. */
int init_lexerSuite(void) {
    assert(0 == _yylval.num);
    assert(0 == _yylloc.first_line);

    return 0;
}

/* Lexer test suite cleanup function. */
int clean_lexerSuite(void) {
    memset(&_yylval, 0, sizeof(YYSTYPE));
    memset(&_yylloc, 0, sizeof(YYLTYPE));

    return 0;
}

#define test_digit(buf, expected_value) do {            \
        CU_ASSERT(NUM == _lex_me(buf));                 \
        DEBUG(TEST_LOGGER, "num %ld", *_yylval.num);    \
        CU_ASSERT((unsigned long)expected_value == (unsigned long)*_yylval.num);      \
        BXIFREE(_yylval.num);                           \
        _lex_clean();                                   \
    } while (0)

#define _test_str(buf, expected_tok, expected_value) do {       \
        CU_ASSERT(expected_tok == _lex_me(buf));                \
        CU_ASSERT_STRING_EQUAL(expected_value, _yylval.str);    \
        BXIFREE(_yylval.str);                   \
        _lex_clean();                                           \
    } while (0)

#define test_tuple(buf, expected_value) do {                    \
        CU_ASSERT(TUPLE == _lex_me(buf));                       \
        bxivector_compare(expected_value, _yylval.tuple, long); \
        bxivector_destroy(&_yylval.tuple, (void (*)(void **))bximem_destroy);\
        _lex_clean();                                           \
    } while (0)

#define test_prefix(buf, expected_value) _test_str(buf, PREFIX, expected_value)

#define test_key(buf, expected_value) _test_str(buf, KEY, expected_value)

#define test_string(buf, expected_value) _test_str(buf, STR, expected_value)

#define test_eof(buf) do {              \
        CU_ASSERT(0 == _lex_me(buf));   \
        _lex_clean();                   \
    } while (0)

#define test_err(buf, expected_mistoken) do {           \
        CU_ASSERT(expected_mistoken == _lex_me(buf));   \
        _lex_clean();                                   \
    } while(0)

/* Simple token stringification */
char *tok2str(enum yytokentype tok) {
    switch(tok) {
        case PREFIX:  return "PREFIX";
        case KEY:  return "KEY";
        case NUM: return "NUM";
        case STR: return "STR";
        case TUPLE: return "TUPLE";
        default: return "/!\\UNKNOWN/*\\";
    }
}

enum yytokentype _lex_me(char *buf) {
    fakefile = fmemopen(buf, BXISTR_BYTES_NB(buf), "r");
    assert(fakefile != NULL);

    _scanner = kvl_init_from_fd(fakefile, "lex_unit_t", NULL);

    enum yytokentype token = yylex(&_yylval, &_yylloc, _scanner);
    DEBUG(TEST_LOGGER, "%s: %s", buf, tok2str(token));
 
    return token;
}

void _lex_clean() {
    kvl_finalize(_scanner);
    fclose(fakefile);
}

void lexSpecial(void) {
    test_eof(" \0");
}

void lexKey(void) {
    /* case */
    test_key("kirikou", "kirikou");

    /* underscore */
    test_key("ki_ik_u", "ki_ik_u");
    test_err("_underscored", '_');

    /* dash */
    // Will be caught by grammar: test_err("dash-", '-');
    // Will be caught by grammar: test_err("da-sh", '-');
    test_err("--dash", '-');
}

void lexPrefix(void) {
    /* case */
    test_prefix("KIRIKOU", "KIRIKOU");
    test_prefix("Kirikou", "K");

    /* underscore */
    test_prefix("KI_IK_U", "KI");
}

void lexDigit(void) {
    char *tst_str = NULL;

    test_digit("0", 0);
    test_digit("-3", -3);

    /* We do not handle float, yet */
    test_err(".414", '.');

    /* The answer to the ultimate question of life, the universe and everything */
    test_digit("42", 42);

    /* UCHAR_MAX = 255 */
    test_digit(tst_str = bxistr_new("%d", UCHAR_MAX), UCHAR_MAX);
    BXIFREE(tst_str);
    test_digit(tst_str = bxistr_new("%d", UCHAR_MAX+1), UCHAR_MAX+1);
    BXIFREE(tst_str);

    /* USHRT_MAX = 65535 */
    test_digit(tst_str = bxistr_new("%d", USHRT_MAX), USHRT_MAX);
    BXIFREE(tst_str);
    test_digit(tst_str = bxistr_new("%d", USHRT_MAX+1), USHRT_MAX+1);
    BXIFREE(tst_str);

    /* UINT_MAX = 4294967295U */
    test_digit(tst_str = bxistr_new("%u", UINT_MAX), UINT_MAX);
    BXIFREE(tst_str);
    test_digit(tst_str = bxistr_new("%lu", (long unsigned int)(UINT_MAX)+1U), (long unsigned int)(UINT_MAX)+1U);
    BXIFREE(tst_str);

    /* LONG_MAX = 9223372036854775807L */
    test_digit(tst_str = bxistr_new("%ld", LONG_MAX), LONG_MAX);
    BXIFREE(tst_str);


    // those one overflow...
    test_digit(tst_str = bxistr_new("%lu", (unsigned long)(LONG_MAX)+1UL), (unsigned long)((LONG_MAX)));
    BXIFREE(tst_str);
    /* ULONG_MAX = 18446744073709551615UL */
    test_digit(tst_str = bxistr_new("%lu", ULONG_MAX), (long)LONG_MAX);
    BXIFREE(tst_str);
    // FIXME: errno?
    test_digit(tst_str = bxistr_new("%llu", (unsigned long long)(ULONG_MAX)+1ULL), 0L);
    BXIFREE(tst_str);
}

void lexString(void) {
    /* Empty string */
    test_string("''", "");
    test_string("\"\"", "");

    /* Simple string */
    test_string("'ehlo'", "ehlo");
    test_string("\"ehlo\"", "ehlo");

    /* Separator */
    test_string("' inner space     do not               count'", " inner space     do not               count");
    test_string("\"inner quote shouldn't count...\"", "inner quote shouldn't count...");
    test_string("'\"'", "\"");
    test_string("\"'\"", "'");

    /* Error */
    /* yylex returns 0 when EOF is reached */
    test_err("'enclose mismatch\"", 0);
    test_err("\"enclose mismatch'", 0);

    test_err("'multi\nline str'", 0);

    test_err("\"", '\0');
    test_err("'", '\0');
}

void lexTuple(void) {
    bxivector_p ttuple = bxivector_new(0, NULL);
    /* Empty tuple */
    test_tuple("()", ttuple);
    test_tuple("{}", ttuple);

    /* Error */
    test_err(")", ')');
    test_err("}", '}');

    test_err("(", '\0');
    test_err("{", '\0');

    test_err("(,", ',');
    test_err("{,", ',');

    test_err("(2,", '\0');
    test_err("{3,", '\0');

    test_err("{)", ')');
    test_err("{2,)", ')');

    test_err("(}", '}');
    test_err("(3,}", '}');


    /* single */
    long *quarante_deux = bximem_calloc(sizeof(long)); *quarante_deux = 42;
    bxivector_push(ttuple, quarante_deux);
    test_tuple("(42)",      ttuple);
    test_tuple("{42}",      ttuple);
    test_tuple("(42 )",     ttuple);
    test_tuple("( 42)",     ttuple);
    test_tuple("{ 42 }",    ttuple);
    test_tuple("{42,}",     ttuple);
    test_tuple("(42, )",    ttuple);
    test_tuple("( 42,   )", ttuple);

    /* double */
    long *dix_huit = bximem_calloc(sizeof(long)); *dix_huit = -18;
    bxivector_push(ttuple, dix_huit);
    test_tuple("(42,-18)",       ttuple);
    test_tuple("(42, -18)",      ttuple);
    test_tuple("(42,-18 )",      ttuple);
    test_tuple("( 42,-18,)",     ttuple);
    test_tuple("( 42,-18, )",    ttuple);

    /* limits */
    long *zero = bximem_calloc(sizeof(long)); *zero = 0;
    bxivector_p zerotuple = bxivector_new(0, NULL);
    bxivector_push(zerotuple, zero);
    test_tuple("(0)",      zerotuple);

    /* clean-up */
    bxivector_destroy(&zerotuple, (void (*)(void **))bximem_destroy);
    bxivector_destroy(&ttuple, (void (*)(void **))bximem_destroy);
}

