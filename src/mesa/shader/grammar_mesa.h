#ifndef GRAMMAR_MESA_H
#define GRAMMAR_MESA_H


#include "imports.h"
/* NOTE: include Mesa 3-D specific headers here */


typedef GLuint grammar;
typedef GLubyte byte;


#define GRAMMAR_PORT_INCLUDE 1
#include "grammar.h"
#undef GRAMMAR_PORT_INCLUDE


#endif

