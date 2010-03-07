/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef SL_PP_TOKEN_H
#define SL_PP_TOKEN_H


struct sl_pp_context;

enum sl_pp_token {
   SL_PP_WHITESPACE,
   SL_PP_NEWLINE,
   SL_PP_HASH,             /* #   */

   SL_PP_COMMA,            /* ,   */
   SL_PP_SEMICOLON,        /* ;   */
   SL_PP_LBRACE,           /* {   */
   SL_PP_RBRACE,           /* }   */
   SL_PP_LPAREN,           /* (   */
   SL_PP_RPAREN,           /* )   */
   SL_PP_LBRACKET,         /* [   */
   SL_PP_RBRACKET,         /* ]   */
   SL_PP_DOT,              /* .   */
   SL_PP_INCREMENT,        /* ++  */
   SL_PP_ADDASSIGN,        /* +=  */
   SL_PP_PLUS,             /* +   */
   SL_PP_DECREMENT,        /* --  */
   SL_PP_SUBASSIGN,        /* -=  */
   SL_PP_MINUS,            /* -   */
   SL_PP_BITNOT,           /* ~   */
   SL_PP_NOTEQUAL,         /* !=  */
   SL_PP_NOT,              /* !   */
   SL_PP_MULASSIGN,        /* *=  */
   SL_PP_STAR,             /* *   */
   SL_PP_DIVASSIGN,        /* /=  */
   SL_PP_SLASH,            /* /   */
   SL_PP_MODASSIGN,        /* %=  */
   SL_PP_MODULO,           /* %   */
   SL_PP_LSHIFTASSIGN,     /* <<= */
   SL_PP_LSHIFT,           /* <<  */
   SL_PP_LESSEQUAL,        /* <=  */
   SL_PP_LESS,             /* <   */
   SL_PP_RSHIFTASSIGN,     /* >>= */
   SL_PP_RSHIFT,           /* >>  */
   SL_PP_GREATEREQUAL,     /* >=  */
   SL_PP_GREATER,          /* >   */
   SL_PP_EQUAL,            /* ==  */
   SL_PP_ASSIGN,           /* =   */
   SL_PP_AND,              /* &&  */
   SL_PP_BITANDASSIGN,     /* &=  */
   SL_PP_BITAND,           /* &   */
   SL_PP_XOR,              /* ^^  */
   SL_PP_BITXORASSIGN,     /* ^=  */
   SL_PP_BITXOR,           /* ^   */
   SL_PP_OR,               /* ||  */
   SL_PP_BITORASSIGN,      /* |=  */
   SL_PP_BITOR,            /* |   */
   SL_PP_QUESTION,         /* ?   */
   SL_PP_COLON,            /* :   */

   SL_PP_IDENTIFIER,

   SL_PP_UINT,
   SL_PP_FLOAT,

   SL_PP_OTHER,

   SL_PP_PRAGMA_OPTIMIZE,
   SL_PP_PRAGMA_DEBUG,

   SL_PP_EXTENSION_REQUIRE,
   SL_PP_EXTENSION_ENABLE,
   SL_PP_EXTENSION_WARN,
   SL_PP_EXTENSION_DISABLE,

   SL_PP_LINE,

   SL_PP_EOF
};

union sl_pp_token_data {
   int identifier;
   int _uint;
   int _float;
   char other;
   int pragma;
   int extension;
   struct {
      unsigned int lineno: 24;
      unsigned int fileno: 8;
   } line;
};

struct sl_pp_token_info {
   enum sl_pp_token token;
   union sl_pp_token_data data;
};

struct sl_pp_purify_options;

int
sl_pp_token_get(struct sl_pp_context *context,
                struct sl_pp_token_info *out);

int
sl_pp_tokenise(struct sl_pp_context *context,
               struct sl_pp_token_info **output);

#endif /* SL_PP_TOKEN_H */
