#ifndef SEXPR_INCLUDED_H
#define SEXPR_INCLUDED_H

#ifdef SEXPR_STATIC
#define SEXPR_DEF static
#else
#define SEXPR_DEF
#endif

#include <stdbool.h>
#include <stdint.h>

typedef enum SExprType {
    SE_NIL,
    SE_BOOL,
    SE_INT,
    SE_FLOAT,
    SE_ID,
    SE_STRING,
    SE_LIST
} SExprType;

// 'start' is an index into the source string
typedef struct SExprString {
    int start, len;
} SExprString;

typedef struct SExpr {
    SExprType type;

    // for SE_LIST elements
    struct SExpr* next;

    union
    {
        bool b;
        int i;
        float f;
        
        // For strings as well as IDs
        SExprString s;

        struct SExpr* head;
    };
} SExpr;

typedef struct SExprPool {
    size_t count;
    SExpr* data;
} SExprPool;

typedef enum SExprResultType {
    SE_SUCCESS,
    SE_COUNT,
    SE_INSUFFICIENT_SPACE,
    SE_SYNTAX_ERROR
} SExprResultType;

typedef struct SExprResult {
    SExprResultType type;
    
    union
    {
        SExpr* expr;

        size_t count;

        struct
        {
            const char* message;
            int lineNumber;
        } syntaxError;
    };
} SExprResult;

// Pass NULL into pool and this will count the number of SExpr you need to allocate.
// If it parses successfully, the top-level sexpr is stored in result.expr
SExprResult ParseSExpr(const char* src, size_t len, SExprPool* pool);

inline static bool SExprStringEqual(const char* src, const SExprString* a, const char* b)
{
    for(int i = 0; i < a->len; ++i) {
        if(!b[i] || src[a->start + i] != b[i]) {
            return false;
        }
    }

    return true;
}

#endif

#ifdef SEXPR_IMPLEMENTATION

#include <ctype.h>
#include <assert.h>
#include <stdlib.h>

typedef struct {
    const char* src;
    size_t len;
    int pos;

    int last;

    int lineNumber;

    SExprPool* pool;
    size_t poolUsed;

    SExprResult result;
} SExprParser;

SEXPR_DEF int SExprGetChar(SExprParser* parser)
{
    return parser->pos >= parser->len ? 0 : parser->src[parser->pos++];
}

SEXPR_DEF int SExprPeek(SExprParser* parser)
{
    return parser->pos >= parser->len ? 0 : parser->src[parser->pos];
}

SEXPR_DEF SExpr* SExprAlloc(SExprParser* parser, SExprType type)
{
    static SExpr dummy;

    if(!parser->pool) {
        ++parser->poolUsed;
        return &dummy;
    }

    if(parser->poolUsed >= parser->pool->count) {
        parser->result.type = SE_INSUFFICIENT_SPACE;
        return NULL;
    }

    SExpr* expr = &parser->pool->data[parser->poolUsed++];

    expr->type = type;
    expr->next = NULL;

    return expr;
}

SEXPR_DEF SExpr* SExprParseValue(SExprParser* parser)
{
    while(parser->last && isspace(parser->last)) {
        if(parser->last == '\n') {
            parser->lineNumber++;
        }

        parser->last = SExprGetChar(parser);
    }

    if(!parser->last) {
        parser->result.type = SE_SYNTAX_ERROR;
        parser->result.syntaxError.message = "Unexpected EOF";

        return NULL;
    }

    if(isalpha(parser->last)) {
        SExprString s;

        s.len = 0;
        s.start = parser->pos - 1;

        while(isalnum(parser->last) || parser->last == '-' ||
                parser->last == '+' || parser->last == '/' ||
                parser->last == '*' || parser->last == '$' ||
                parser->last == '?' || parser->last == '_' ||
                parser->last == '=') {
            s.len++;
            parser->last = SExprGetChar(parser);
        }

        if(SExprStringEqual(parser->src, &s, "true")) {
            static SExpr strue = {SE_BOOL};
            strue.b = true;

            return &strue;
        } else if(SExprStringEqual(parser->src, &s, "false")) {
            static SExpr sfalse = {SE_BOOL};
            sfalse.b = false;
            
            return &sfalse;
        }

        SExpr* expr = SExprAlloc(parser, SE_ID);
        expr->s = s;

        return expr;
    } else if(isdigit(parser->last) || (parser->last == '-' && isdigit(SExprPeek(parser)))) {
        int i = 0;
        char buf[32];

        bool isFloat = false;

        while(isdigit(parser->last) || (!isFloat && parser->last == '.') || parser->last == '-') {
            if(i >= sizeof(buf) - 1) {
                parser->result.type = SE_SYNTAX_ERROR;
                parser->result.syntaxError.message = "Numeric literal is too long.";
                return NULL;
            }

            if(parser->last == '.') {
                isFloat = true;
            }

            buf[i++] = parser->last;
            parser->last = SExprGetChar(parser);
        }

        buf[i] = '\0';

        if(isFloat) {
            SExpr* expr = SExprAlloc(parser, SE_FLOAT);
            expr->f = (float)strtod(buf, NULL);
            return expr;
        }    

        SExpr* expr = SExprAlloc(parser, SE_INT);
        expr->i = strtol(buf, NULL, 10);
        return expr;
    } else if(parser->last == '"') {
        SExpr* expr = SExprAlloc(parser, SE_STRING);

        expr->s.len = 0;
        expr->s.start = parser->pos;

        parser->last = SExprGetChar(parser);

        while(parser->last && parser->last != '"') {
            expr->s.len++;
            parser->last = SExprGetChar(parser);
        }

        if(!parser->last) {
            parser->result.type = SE_SYNTAX_ERROR;
            parser->result.syntaxError.message = "Unexpected EOF when expecting end of string.";
            return NULL;
        }

        parser->last = SExprGetChar(parser);

        return expr;
    } else if(parser->last == '(' || parser->last == '[' || parser->last == '{') {
        parser->last = SExprGetChar(parser);

        while(parser->last && isspace(parser->last)) {
			if(parser->last == '\n') {
				parser->lineNumber++;
			}
            parser->last = SExprGetChar(parser);
        }

        if(!parser->last) {
            parser->result.type = SE_SYNTAX_ERROR;
            parser->result.syntaxError.message = "Unexpected EOF when expecting ')' or list elements.";
            return NULL;
        }

        if(parser->last == ')' || parser->last == ']' || parser->last == '}') {
            parser->last = SExprGetChar(parser);
            
            static SExpr nil = {SE_NIL};
            return &nil;
        }

        SExpr* list = SExprAlloc(parser, SE_LIST);
        SExpr* tail = NULL;

        while(parser->last && parser->last != ')' && parser->last != ']' && parser->last != '}') {
            SExpr* elem = SExprParseValue(parser);

            if(!elem) {
                // There must have been an error
                return NULL;
            }

            if(!tail) {
                list->head = tail = elem;
            } else {
                tail->next = elem;
				tail = elem;
            }

			while(parser->last && isspace(parser->last)) {		
				if(parser->last == '\n') {
					parser->lineNumber++;
				}
				parser->last = SExprGetChar(parser);
			}
        }

        if(!parser->last) {
            parser->result.type = SE_SYNTAX_ERROR;
            parser->result.syntaxError.message = "Unexpected EOF when expecting end of list.";
            return NULL;
        }

        parser->last = SExprGetChar(parser);

        return list;
    }

    parser->result.type = SE_SYNTAX_ERROR;
    parser->result.syntaxError.message = "Unexpected character";
    return NULL;
}

SEXPR_DEF SExprResult ParseSExpr(const char* src, size_t len, SExprPool* pool)
{
    SExprParser parser;

    parser.src = src;
    parser.len = len;
    parser.pos = 0;

    parser.last = ' ';

    parser.lineNumber = 1;

    parser.pool = pool;
    parser.poolUsed = 0;

    parser.result.type = SE_SUCCESS;

    if(len == 0 || (src[0] != '(' && src[0] != '[' && src[0] != '{')) {
        parser.result.type = SE_SYNTAX_ERROR;

        parser.result.syntaxError.lineNumber = 1;
        parser.result.syntaxError.message = "Expected list at start of input.";

        return parser.result;
    }

    SExpr* expr = SExprParseValue(&parser);

    if(parser.result.type == SE_SUCCESS) {
        if(!pool) {
            parser.result.type = SE_COUNT;
            parser.result.count = parser.poolUsed;
        } else {
            parser.result.expr = expr;
        }
    } else if(parser.result.type == SE_SYNTAX_ERROR) {
        parser.result.syntaxError.lineNumber = parser.lineNumber;
    }

    return parser.result;
}

#endif