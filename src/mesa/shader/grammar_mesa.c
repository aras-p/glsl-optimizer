#include "grammar_mesa.h"

#define GRAMMAR_PORT_BUILD 1
#include "grammar.c"
#undef GRAMMAR_PORT_BUILD


void grammar_alloc_free (void *ptr)
{
    _mesa_free (ptr);
}

void *grammar_alloc_malloc (unsigned int size)
{
    return _mesa_malloc (size);
}

void *grammar_alloc_realloc (void *ptr, unsigned int old_size, unsigned int size)
{
    return _mesa_realloc (ptr, old_size, size);
}

void *grammar_memory_copy (void *dst, const void * src, unsigned int size)
{
    return _mesa_memcpy (dst, src, size);
}

int grammar_string_compare (const byte *str1, const byte *str2)
{
    return _mesa_strcmp ((const char *) str1, (const char *) str2);
}

int grammar_string_compare_n (const byte *str1, const byte *str2, unsigned int n)
{
    return _mesa_strncmp ((const char *) str1, (const char *) str2, n);
}

byte *grammar_string_copy (byte *dst, const byte *src)
{
    return (byte *) _mesa_strcpy ((char *) dst, (const char *) src);
}

byte *grammar_string_copy_n (byte *dst, const byte *src, unsigned int n)
{
    return (byte *) _mesa_strncpy ((char *) dst, (const char *) src, n);
}

byte *grammar_string_duplicate (const byte *src)
{
    return (byte *) _mesa_strdup ((const char *) src);
}

unsigned int grammar_string_length (const byte *str)
{
    return _mesa_strlen ((const char *) str);
}

