#ifndef GRAMMAR_H
#define GRAMMAR_H


#ifndef GRAMMAR_PORT_INCLUDE
#error Do not include this file directly, include your grammar_XXX.h instead
#endif


#ifdef __cplusplus
extern "C" {
#endif

void grammar_alloc_free (void *);
void *grammar_alloc_malloc (unsigned int);
void *grammar_alloc_realloc (void *, unsigned int, unsigned int);
void *grammar_memory_copy (void *, const void *, unsigned int);
int grammar_string_compare (const byte *, const byte *);
int grammar_string_compare_n (const byte *, const byte *, unsigned int);
byte *grammar_string_copy (byte *, const byte *);
byte *grammar_string_copy_n (byte *, const byte *, unsigned int);
byte *grammar_string_duplicate (const byte *);
unsigned int grammar_string_length (const byte *);

/*
    loads grammar script from null-terminated ASCII <text>
    returns unique grammar id to grammar object
    returns 0 if an error occurs (call grammar_get_last_error to retrieve the error text)
*/
grammar grammar_load_from_text (const byte *text);

/*
    sets a new <value> to a register <name> for grammar <id>
    returns 0 on error (call grammar_get_last_error to retrieve the error text)
    returns 1 on success
*/
int grammar_set_reg8 (grammar id, const byte *name, byte value);

/*
    checks if a null-terminated <text> matches given grammar <id>
    returns 0 on error (call grammar_get_last_error to retrieve the error text)
    returns 1 on success, the <prod> points to newly allocated buffer with production and <size>
    is filled with the production size
    call grammar_alloc_free to free the memory block pointed by <prod>
*/
int grammar_check (grammar id, const byte *text, byte **prod, unsigned int *size);

/*
    destroys grammar object identified by <id>
    returns 0 on error (call grammar_get_last_error to retrieve the error text)
    returns 1 on success
*/
int grammar_destroy (grammar id);

/*
    retrieves last grammar error reported either by grammar_load_from_text, grammar_check
    or grammar_destroy
    the user allocated <text> buffer receives error description, <pos> points to error position,
    <size> is the size of the text buffer to fill in - it must be at least 4 bytes long,
*/
void grammar_get_last_error (byte *text, unsigned int size, int *pos);

#ifdef __cplusplus
}
#endif

#endif

