

#ifndef DRAW_PRIM_H
#define DRAW_PRIM_H


void draw_invalidate_vcache( struct draw_context *draw );

void draw_set_prim( struct draw_context *draw, unsigned prim );

void draw_set_element_buffer( struct draw_context *draw,
                              unsigned eltSize, void *elements );

void draw_prim( struct draw_context *draw, unsigned start, unsigned count );

void draw_flush( struct draw_context *draw );

unsigned draw_prim_info( unsigned prim, unsigned *first, unsigned *incr );

unsigned draw_trim( unsigned count, unsigned first, unsigned incr );


#endif /* DRAW_PRIM_H */
